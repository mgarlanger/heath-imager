#include <gtk/gtk.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>


#include "disk.h"
#include "heath_hs.h"
#include "drive.h"
#include "fc5025.h"
#include "h17disk.h"

#define VERSION_STRING "0.2"

#define DIRECTORY_SEPARATOR "/"


#define MY_NAME "HeathImager"

static GtkWidget              *fname_field;
static GtkWidget              *in_fname_field;
static GtkWidget              *description_field;
static GtkWidget              *outdir_field;
static struct DriveInfo      *selected_drive = NULL;
static GtkWidget              *imgbutton,
                              *diskInfoButton;
GtkWidget                     *listbox,
                              *pop_button;

static int                     errors;
static float                   progress;
static int                     drive_is_opened = 0;

static int		selected_item_type;
static char		*selected_item_name;
static int		modal = 0;
static int		img_cancelled;
static int		diskinfoStop;
static bool		wpDisk = false;
static uint8_t		dist_status = 0;
static uint8_t		side_status = 0;
static uint8_t		track_status = 39;
static uint8_t		tpi_status = 48;
static uint16_t		speed_rpm 	= 300;
static uint16_t		speed_status 	= 300;
static uint16_t		speed_param 	= 5555;
static GtkTextBuffer	*textBuffer;

// unsigned char  nullptr[256];

void
addCommentToFile(H17Disk *image)
{
    GtkTextIter    start, end;
    size_t         length;
    gchar         *text;

    gtk_text_buffer_get_bounds(textBuffer, &start, &end);

    text = gtk_text_buffer_get_text (textBuffer, &start, &end, TRUE); 
    length = strlen(text) + 1; // include null termination
    printf("Size of textBuffer: %d\n", length);

    image->writeComment((unsigned char *)text, (uint32_t) length);

    g_free(text);    
}    

void
refresh_screen(void)
{
    while (gtk_events_pending())
    {
        gtk_main_iteration();
    }
}

gint
disallow_delete(GtkWidget * widget, gpointer gdata)
{
    return TRUE;                /* prevent window from closing */
}

void
toggleWP(GtkWidget * widget, gpointer gdata)
{
    wpDisk = !wpDisk;
}


void
quitpressed(GtkWidget * widget, gpointer gdata)
{
    gtk_main_quit();
}

static void
normalize_path(char *path)
{
    char    *p = path,
            *q;

    while (*p != '\0')
    {
        if (p[0] == '/' && (p[1] == '/' || p[1] == '\0'))
        {
            memmove(p, p + 1, strlen(p + 1) + 1);
            continue;
        }
        q = strchr(p, '/');
        if (q && (!strcmp(q, "/..") || !strncmp(q, "/../", 4)) && q != p)
        {
            p--;
            memmove(p, q + 3, strlen(q + 3) + 1);
            p = path;
            continue;
        }
        p++;
    }
    if (path[0] == '\0')
    {
        strcpy(path, "/");
    }
}

void
destroy_filew(GtkWidget * widget, gpointer gdata)
{
    GtkWidget      *filew = (GtkWidget *) gdata;

    gtk_grab_remove(filew);
}

static void
read_one_sector(H17Disk       *image,
                Disk          *disk,
                int            track,
                int            side,
                int            sector,
                GtkWidget     *error_label)
{
    const int      maxRetries_c = 10;
    unsigned char  buf[HeathHSDisk::defaultSectorBytes()];
    unsigned char  rawBuf[HeathHSDisk::defaultSectorRawBytes()];
    char           errtext[80];
    int            retVal;
    int            retryCount = 0;

    do
    {
        retVal = disk->readSector(buf, rawBuf, side, track, sector);
        if (retVal != 0)
        {
            snprintf(errtext, sizeof(errtext), "Failed on attempt: %d: %d H: %d T: %d S:%d",
                     retryCount, retVal, side, track, sector);
            gtk_label_set(GTK_LABEL(error_label), errtext);
            refresh_screen();
        }
        // If it was a read error, then raw bytes are not valid, otherwise store raw
        if (retVal != Disk::Err_ReadError)
        {
            image->addRawSector(sector, rawBuf, disk->sectorRawBytes(side, track, sector));
        }
    }
    while ((retVal != 0) && (retryCount++ < maxRetries_c));

    // even if there is an error, use the last processed sector for storage, unless
    // it was a read error.
    //
    if (retVal == Disk::Err_ReadError)
    {
        image->addSector(sector, retVal, nullptr, 0);
    }
    else
    {
        image->addSector(sector, retVal, buf, disk->sectorBytes(side, track, sector));
    }
    if (retVal != 0)
    {
        snprintf(errtext, sizeof(errtext), "Failed after %d attempts: %d H: %d T: %d S:%d",
                 maxRetries_c, retVal, side, track, sector);
        gtk_label_set(GTK_LABEL(error_label), errtext);
        refresh_screen();
    }
    else
    {
        snprintf(errtext, sizeof(errtext), "Passed after %d attempts -  H: %d T: %d S:%d",
                 retryCount, side, track, sector);
        gtk_label_set(GTK_LABEL(error_label), errtext);
        refresh_screen();
    }

}

void
increment_progressbar(GtkWidget * progressbar, float progress_per_halftrack)
{
    progress += progress_per_halftrack;
    if (progress > 1.0)
    {
        progress = 1.0;
    }
    gtk_progress_set_percentage(GTK_PROGRESS(progressbar), progress);
}

int
image_track(H17Disk    *image,
            Disk       *disk,
            int         track,
            int         side,
            GtkWidget  *status_label,
            GtkWidget  *error_label,
            GtkWidget  *progressbar,
            float       progress_per_halftrack)
{
    unsigned char      *buf;
    unsigned char      *raw;
    SectorList         *sector_list,
                       *sector_entry;
    int                 num_sectors = disk->numSectors(track, side);
    int                 halfway_mark = num_sectors >> 1;
    std::vector<RawSector *> rawSectors;
    std::vector<Sector *>    sectors;

    if (FC5025::inst()->seek(disk->physicalTrack(track)) != 0)
    {
        gtk_label_set(GTK_LABEL(status_label), "Unable to seek to track! Giving up.");
        return 1;
    }
    refresh_screen();
    buf = (unsigned char *) malloc(disk->trackBytes(track, side));
    if (!buf)
    {
        gtk_label_set(GTK_LABEL(status_label), "Out of memory! Giving up.");
        return 1;
    }

    raw = (unsigned char *) malloc(disk->trackRawBytes(track, side));
    if (!raw)
    {
        gtk_label_set(GTK_LABEL(status_label), "Out of memory! Giving up.");
        return 1;
    }

    sector_list = (SectorList *) malloc(sizeof(SectorList) * num_sectors);
    if (!sector_list)
    {
        gtk_label_set(GTK_LABEL(status_label), "Out of memory! Giving up.");
        free(buf);
        return 1;
    }

    disk->genBestReadOrder(sector_list, track, side);

    sector_entry = sector_list;
    image->startTrack(side, track);

    while (num_sectors--)
    {
        printf("%s: reading one sector: %d\n", __FUNCTION__, sector_entry->sector);        
        read_one_sector(image, disk, track, side, sector_entry->sector, error_label);
        if (img_cancelled)
        {
            gtk_label_set(GTK_LABEL(status_label), "Cancelled.");
            free(sector_list);
            free(buf);
            return 1;
        }
        if (num_sectors == halfway_mark)
        {
            increment_progressbar(progressbar, progress_per_halftrack);
            refresh_screen();
        }
        sector_entry++;
    }
    free(sector_list);
    increment_progressbar(progressbar, progress_per_halftrack);
    refresh_screen();

    image->endTrack();
    refresh_screen();
    free(buf);
    return 0;
}


/// \todo make sure file does not already exist.
void
auto_increment_filename(void)
{
    const char     *old_filename = gtk_entry_get_text(GTK_ENTRY(fname_field));
    char           *new_filename;
    char           *p;

    p = (char *) strrchr(old_filename, '.');
    if (!p || (p == old_filename))
        return;
    new_filename = strdup(old_filename);
    if (!new_filename)
    {
        return;
    }
    p = strrchr(new_filename, '.') - 1;
    if (!isdigit(*p))
    {
        free(new_filename);
        return;
    }
    (*p)++;
    while ((*p == '9' + 1) && (p != new_filename) && isdigit(*(p - 1)))
    {
        *p = '0';
        p--;
        (*p)++;
    }
    if (*p == '9' + 1)
    {
        *p = 'X';
    }
    gtk_entry_set_text(GTK_ENTRY(fname_field), new_filename);
    free(new_filename);
}

void
imgdone(GtkWidget * widget, gpointer gdata)
{
    GtkWidget      *image_window = (GtkWidget *) gdata;

    gtk_widget_destroy(GTK_WIDGET(image_window));
}

void
diskInfoDone(GtkWidget * widget, gpointer gdata)
{
    GtkWidget      *image_window = (GtkWidget *) gdata;

    gtk_widget_destroy(GTK_WIDGET(image_window));
}

void
diskInfoStop(GtkWidget *widget, gpointer gdata)
{
    diskinfoStop = 1;
}

void
imgcancel(GtkWidget * widget, gpointer gdata)
{
    img_cancelled = 1;
}

void
destroy_img(GtkWidget * widget, gpointer gdata)
{
    GtkWidget      *image_window = (GtkWidget *) gdata;

    gtk_grab_remove(image_window);
    auto_increment_filename();
}

void
imgFailed(GtkWidget * image_window, gint delete_signal, GtkWidget * status_label,
          GtkWidget * button_label, GtkWidget * button, GtkWidget * cancelbutton, char *text)
{
    if (text)
    {
        gtk_label_set(GTK_LABEL(status_label), text);
    }
    gtk_label_set(GTK_LABEL(button_label), "Bummer.");
    gtk_widget_set_sensitive(cancelbutton, 0);
    gtk_widget_set_sensitive(button, 1);
    gtk_signal_disconnect(GTK_OBJECT(image_window), delete_signal);
    modal = 0;
}

void
destroy_diskinfo(GtkWidget *widget, gpointer gdata)
{
    GtkWidget      *diskInfoWindow = (GtkWidget *) gdata;

    gtk_grab_remove(diskInfoWindow);
}

void 
diskInfoPressed(GtkWidget * widget, gpointer gdata)
{
    GtkWidget      *diskInfoWindow   = gtk_dialog_new();
    GtkWidget      *button = gtk_button_new();
    GtkWidget      *stopButton       = gtk_button_new_with_label("Stop");
    GtkWidget      *doneButton       = gtk_button_new_with_label("Done");
    GtkWidget      *trackLabel       = gtk_label_new("Track:   --\n");
    GtkWidget      *speedLabel       = gtk_label_new("RPM:     --\n");
    GtkWidget      *sectorCountLabel = gtk_label_new("Sectors: --\n");
    GtkWidget      *flagsLabel       = gtk_label_new("Flags:   --\n");
    GtkWidget      *errorLabel       = gtk_label_new("");
    char            status_text[80];
    uint8_t         track, sectorCount, flags;
    uint16_t        speed;
    int             retVal;
    gint            delete_signal;
    Drive           drive(selected_drive);

    diskinfoStop = 0;

    gtk_window_set_title(GTK_WINDOW(diskInfoWindow), "Disk and Drive Info");
    delete_signal = gtk_signal_connect(GTK_OBJECT(diskInfoWindow), "delete_event",
                                       GTK_SIGNAL_FUNC(disallow_delete), NULL);
    gtk_signal_connect(GTK_OBJECT(diskInfoWindow), "destroy", (GtkSignalFunc) destroy_diskinfo,
                       (gpointer) diskInfoWindow);
    modal = 1;
    gtk_container_border_width(GTK_CONTAINER(diskInfoWindow), 10);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(diskInfoWindow)->vbox), trackLabel, TRUE, TRUE, 0);
    gtk_widget_show(trackLabel);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(diskInfoWindow)->vbox), speedLabel, TRUE, TRUE, 0);
    gtk_widget_show(speedLabel);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(diskInfoWindow)->vbox), sectorCountLabel, TRUE, TRUE, 0);
    gtk_widget_show(sectorCountLabel);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(diskInfoWindow)->vbox), flagsLabel, TRUE, TRUE, 0);
    gtk_widget_show(flagsLabel);


    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(diskInfoWindow)->vbox), errorLabel, TRUE, TRUE, 0);
    gtk_widget_show(errorLabel);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(diskInfoWindow)->action_area), doneButton, TRUE,
                       TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(doneButton), "clicked", GTK_SIGNAL_FUNC(diskInfoDone),
                       diskInfoWindow);
    gtk_widget_set_sensitive(doneButton, 0);
    gtk_widget_show(doneButton);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(diskInfoWindow)->action_area), stopButton, TRUE,
                       TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(stopButton), "clicked", GTK_SIGNAL_FUNC(diskInfoStop),
                       diskInfoWindow);
    gtk_widget_set_sensitive(stopButton, 1);
    gtk_widget_show(stopButton);


    gtk_widget_show(diskInfoWindow);
    gtk_grab_add(diskInfoWindow);
    refresh_screen();

//    drive = new Drive(selected_drive);
 
    if (drive.getStatus() != 0)
    {
       // gtk_label_set(GTK_LABEL(errorLabel), "Unable to open drive");
        return;
    }
    while (!diskinfoStop)
    { 
        if ((retVal = FC5025::inst()->driveStatus(&track, &speed, &sectorCount, &flags)) != 0)
        {
            gtk_label_set(GTK_LABEL(errorLabel), "Unable to get drive status");
            gtk_widget_set_sensitive(doneButton, 1);
            gtk_widget_set_sensitive(stopButton, 0);
            gtk_signal_disconnect(GTK_OBJECT(diskInfoWindow), delete_signal);
            modal = 0;
            return;
        }
        snprintf(status_text, sizeof(status_text), "Track:        %02d\n", track);
        gtk_label_set(GTK_LABEL(trackLabel), status_text);
        snprintf(status_text, sizeof(status_text), "Speed:        %6.2f\n", speed/100.0);
        gtk_label_set(GTK_LABEL(speedLabel), status_text);
        snprintf(status_text, sizeof(status_text), "Sector Count: %02d\n", sectorCount);
        gtk_label_set(GTK_LABEL(sectorCountLabel), status_text);
        snprintf(status_text, sizeof(status_text), "Flags:        0x%02x\n", flags);
        gtk_label_set(GTK_LABEL(flagsLabel), status_text);
        refresh_screen();
        usleep(100000);
    } 
    gtk_widget_set_sensitive(doneButton, 1);
    gtk_widget_set_sensitive(stopButton, 0);
    gtk_signal_disconnect(GTK_OBJECT(diskInfoWindow), delete_signal);
    modal = 0;
}
/// Start imaging a disk.
void
imgPressed(GtkWidget * widget, gpointer gdata)
{
    GtkWidget      *image_window = gtk_dialog_new();
    GtkAdjustment  *adj = (GtkAdjustment *) gtk_adjustment_new(0, 0, 400, 0, 0, 0);
    GtkWidget      *progressbar = gtk_progress_bar_new_with_adjustment(adj);
    GtkWidget      *button = gtk_button_new();
    GtkWidget      *cancelbutton = gtk_button_new_with_label("Cancel");
    char            status_text[80];
    GtkWidget      *status_label = gtk_label_new("Preparing...");
    GtkWidget      *error_label = gtk_label_new("");
    GtkWidget      *button_label = gtk_label_new("In progress...");
    float           progress_per_halftrack;
    //
    // DSE:   Next line modified to add parameter for disk rotation speed - adjusts RPMparam
    // DSE:   Required modification of heath_hs.cpp/heath_hs.h in src/libs and re-make there
    // DSE:     original line
    // DSE:    Disk           *disk = new HeathHSDisk(side_status, track_status, tpi_status);
    // DSE:     modified line
    Disk           *disk = new HeathHSDisk(side_status, track_status, tpi_status, speed_status);
    int             track,
                    side;
    char           *in_filename;
    char           *out_filename;
    //char           *out_rawFilename;
    FILE           *f;
    FILE           *f_raw;
    gint            delete_signal;
    H17Disk        *image;
    Drive          drive(selected_drive);
    bool           recovery;

    progress = 0;
    errors = 0;

    gtk_window_set_title(GTK_WINDOW(image_window), "Capturing Disk Image File...");
    delete_signal = gtk_signal_connect(GTK_OBJECT(image_window), "delete_event",
                                       GTK_SIGNAL_FUNC(disallow_delete), NULL);
    gtk_signal_connect(GTK_OBJECT(image_window), "destroy", (GtkSignalFunc) destroy_img,
                       (gpointer) image_window);
    modal = 1;
    gtk_container_border_width(GTK_CONTAINER(image_window), 10);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(image_window)->vbox), status_label, TRUE, TRUE, 0);
    gtk_widget_show(status_label);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(image_window)->vbox), error_label, TRUE, TRUE, 0);
    gtk_widget_show(error_label);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(image_window)->vbox), progressbar, TRUE, TRUE, 0);
    gtk_progress_set_percentage(GTK_PROGRESS(progressbar), 0);
    gtk_widget_show(progressbar);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(image_window)->action_area), cancelbutton, TRUE,
                       TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(cancelbutton), "clicked", GTK_SIGNAL_FUNC(imgcancel),
                       image_window);
    gtk_widget_set_sensitive(cancelbutton, 0);
    gtk_widget_show(cancelbutton);

    gtk_container_add(GTK_CONTAINER(button), button_label);
    gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(imgdone), image_window);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(image_window)->action_area), button, TRUE, TRUE, 0);
    gtk_widget_set_sensitive(button, 0);
    gtk_widget_show(button_label);
    gtk_widget_show(button);

    gtk_widget_show(image_window);
    gtk_grab_add(image_window);
    refresh_screen();

//    drive = new Drive(selected_drive);
   
    if (drive.getStatus() != 0)
    {
        imgFailed(image_window, delete_signal, status_label, button_label,
                  button, cancelbutton, (char *) "Unable to open drive.");
        return;
    }
    refresh_screen();

    if (strlen(gtk_entry_get_text(GTK_ENTRY(in_fname_field))) != 0)
    {
        recovery = true;
    } 
    if (recovery)
    {
        in_filename = (char *) malloc(strlen(gtk_entry_get_text(GTK_ENTRY(outdir_field))) + 1 +
                              strlen(gtk_entry_get_text(GTK_ENTRY(in_fname_field))) + 1);
        if (!in_filename)
        {
            imgFailed(image_window, delete_signal, status_label, button_label,
                      button, cancelbutton, (char *) "Out of memory!");
            return;
        }
        strcpy(in_filename, gtk_entry_get_text(GTK_ENTRY(outdir_field)));
        strcat(in_filename, DIRECTORY_SEPARATOR);
        strcat(in_filename, gtk_entry_get_text(GTK_ENTRY(in_fname_field)));
    }

    out_filename = (char *) malloc(strlen(gtk_entry_get_text(GTK_ENTRY(outdir_field))) + 1 +
                          strlen(gtk_entry_get_text(GTK_ENTRY(fname_field))) + 1);
    if (!out_filename)
    {
        imgFailed(image_window, delete_signal, status_label, button_label,
                  button, cancelbutton, (char *) "Out of memory!");
        return;
    }
    strcpy(out_filename, gtk_entry_get_text(GTK_ENTRY(outdir_field)));
    strcat(out_filename, DIRECTORY_SEPARATOR);
    strcat(out_filename, gtk_entry_get_text(GTK_ENTRY(fname_field)));
    image = new H17Disk();
   
    image->openForRecovery(in_filename);
 
    image->openForWrite(out_filename);
    if (FC5025::inst()->recalibrate() != 0)
    {
        fclose(f);
        imgFailed(image_window, delete_signal, status_label, button_label,
                  button, cancelbutton, (char *) "Unable to recalibrate drive.");
        return;
    }
    refresh_screen();

    if (FC5025::inst()->setDensity(disk->density()) != 0)
    {
        fclose(f);
        imgFailed(image_window, delete_signal, status_label, button_label,
                  button, cancelbutton, (char *) "Unable to set density.");
        return;
    }
    refresh_screen();

    // print file header for this disk
    // 
    // 
    // -> phys->writeHeader(
    image->writeHeader();
    image->setSides(disk->numSides());
    image->setTracks(disk->numTracks());
    image->writeDiskFormatBlock();

    image->setWPParameter(wpDisk);
    image->setDistributionParameter(dist_status);
    image->setTrackDataParameter(3);
    image->writeParameters();
    addCommentToFile(image);
    image->startData();

    img_cancelled = 0;
    gtk_widget_set_sensitive(cancelbutton, 1);
    refresh_screen();

    progress_per_halftrack = (float) 1 / (float) (disk->numTracks() * 
                             disk->numSides() * 2);

    for (track = disk->minTrack(); track <= disk->maxTrack(); track++)
    {
        for (side = disk->minSide(); side <= disk->maxSide(); side++)
        {
            if (disk->numSides() == 1)
            {
                snprintf(status_text, sizeof(status_text), "Reading track %d...\n", track);
            }
            else
            {
                snprintf(status_text, sizeof(status_text), "Reading track %d side %d...\n",
                         track, side);
            }
            gtk_label_set(GTK_LABEL(status_label), status_text);
            refresh_screen();
            /* image the track */
            if (image_track(image, disk, track, side, status_label, error_label, progressbar,
                            progress_per_halftrack) != 0)
            {
                image->endDataBlock();
                image->writeRawDataBlock();
                image->closeFile();
                imgFailed(image_window, delete_signal, status_label, button_label,
                          button, cancelbutton, NULL);
                return;
            }

        }
    }
    image->endDataBlock();
    image->writeRawDataBlock();

    image->closeFile();

    if (!errors)
    {
        gtk_label_set(GTK_LABEL(status_label), "Successfully read disk.");
        gtk_label_set(GTK_LABEL(button_label), "Yay!");
    }
    else
    {
        gtk_label_set(GTK_LABEL(status_label), "Some sectors did not read correctly.");
        gtk_label_set(GTK_LABEL(button_label), "Bummer.");
    }

    gtk_widget_set_sensitive(cancelbutton, 0);
    gtk_widget_set_sensitive(button, 1);
    gtk_signal_disconnect(GTK_OBJECT(image_window), delete_signal);
    modal = 0;

    return;
}

void
update_sensitivity(void)
{
    if (selected_drive == NULL)
    {
        gtk_widget_set_sensitive(imgbutton, 0);
        gtk_widget_set_sensitive(diskInfoButton, 0);
    }
    else
    {
        gtk_widget_set_sensitive(imgbutton, 1);
        gtk_widget_set_sensitive(diskInfoButton, 1);
    }
}

void
drive_changed(GtkWidget * widget, gpointer data)
{
    selected_drive = (DriveInfo *) data;
    update_sensitivity();
}

void
dist_changed(GtkWidget *widget, gpointer data)
{
    dist_status = *(uint8_t *) data;
    printf("New dist status: %d\n", dist_status);
}

void
wp_changed(GtkWidget *widget, gpointer data)
{
    wpDisk = *(bool *) data;
    printf("New wp: %d\n", wpDisk);
}

void
side_changed(GtkWidget *widget, gpointer data)
{
    side_status = *(uint8_t *) data;
    printf("New side: %d\n", side_status);
}

void
track_changed(GtkWidget *widget, gpointer data)
{
    track_status = *(uint8_t *) data;
    if (track_status == 79)
    {
        tpi_status = 96;
    }
    else
    {
        tpi_status = 48;
    }
    printf("New track/tpi: %d/%d\n", track_status, tpi_status);
}

void
speed_changed(GtkWidget *widget, gpointer data)
{
    speed_status = *(int *) data;
    if (speed_status == 300)
    {
        speed_param = 5555;
    }
    else
    {
        speed_param = 6666;
    }
    printf("New speed: %d (%d)\n", speed_status, speed_param);
}


void
add_side(GtkWidget *menu)
{
    GtkWidget      *mitem;
    GSList         *group = NULL;
    static uint8_t   value[2] = {0, 1};

    mitem = gtk_radio_menu_item_new_with_label(group, "Single-Sided");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate",
                       GTK_SIGNAL_FUNC(side_changed), &value[0]);

    mitem = gtk_radio_menu_item_new_with_label(group, "Double-Sided");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate",
                       GTK_SIGNAL_FUNC(side_changed), &value[1]);
}

void
add_track(GtkWidget *menu)
{   
    GtkWidget      *mitem;
    GSList         *group = NULL;
    static uint8_t   value[2] = {39, 79};

    mitem = gtk_radio_menu_item_new_with_label(group, "40 Track/48 TPI");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate",
                       GTK_SIGNAL_FUNC(track_changed), &value[0]);
    
    mitem = gtk_radio_menu_item_new_with_label(group, "80 Track/96 TPI");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate",
                       GTK_SIGNAL_FUNC(track_changed), &value[1]);
}

void
add_speed(GtkWidget *menu)
{
    GtkWidget      *mitem;
    GSList         *group = NULL;
    static int   value[2] = {300, 360};

    mitem = gtk_radio_menu_item_new_with_label(group, "300 RPM");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate",
                       GTK_SIGNAL_FUNC(speed_changed), &value[0]);

    mitem = gtk_radio_menu_item_new_with_label(group, "360 RPM");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate",
                       GTK_SIGNAL_FUNC(speed_changed), &value[1]);
}



void
add_wp(GtkWidget *menu)
{
    GtkWidget      *mitem;
    GSList         *group = NULL;
    static bool   value[2] = {false, true};

    mitem = gtk_radio_menu_item_new_with_label(group, "Write Enabled");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate", 
                       GTK_SIGNAL_FUNC(wp_changed), &value[0]);

    mitem = gtk_radio_menu_item_new_with_label(group, "Write Protected");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate", 
                       GTK_SIGNAL_FUNC(wp_changed), &value[1]);

}

void
add_distribution(GtkWidget *menu)
{
    GtkWidget      *mitem;
    GSList         *group = NULL;
    static uint8_t value[3] = {0, 1, 2 };

    mitem = gtk_radio_menu_item_new_with_label(group, "Disk Status Unknown");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate",
                       GTK_SIGNAL_FUNC(dist_changed), &value[0]);

    mitem = gtk_radio_menu_item_new_with_label(group, "Distribution Disk");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate",
                       GTK_SIGNAL_FUNC(dist_changed), &value[1]);

    mitem = gtk_radio_menu_item_new_with_label(group, "User Disk");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate",
                       GTK_SIGNAL_FUNC(dist_changed), &value[2]);

}

void
add_drives(GtkWidget * menu)
{
    GtkWidget      *mitem;
    GSList         *group = NULL;
    struct DriveInfo *drive,
                   *drives;
    char            label[100];

    drives = Drive::get_drive_list();
    if (!drives)
    {
        mitem = gtk_radio_menu_item_new_with_label(group, "No drives found.");
        gtk_menu_append(GTK_MENU(menu), mitem);
        gtk_widget_show(mitem);
        return;
    }

    for (drive = drives; drive->id[0] != '\0'; drive++)
    {
        if (selected_drive == NULL)
        {
            selected_drive = drive;
        }
        snprintf(label, sizeof(label), "%s (%s)", drive->desc, drive->id);
        mitem = gtk_radio_menu_item_new_with_label(group, label);
        gtk_menu_append(GTK_MENU(menu), mitem);
        gtk_widget_show(mitem);
        gtk_signal_connect(GTK_OBJECT(mitem), "activate", 
                           GTK_SIGNAL_FUNC(drive_changed), drive);
    }
}

void
populate_outdir(void)
{
    char  local_cwd[1024];

    if (getcwd(local_cwd, sizeof(local_cwd)))
    {
        gtk_entry_set_text(GTK_ENTRY(outdir_field), local_cwd);
    }
    else
    {
        gtk_entry_set_text(GTK_ENTRY(outdir_field), ".");
    }
}


int
main(int argc, char *argv[])
{
    GtkWidget      *window,
                   *vbox,
                   *quitbutton,
                   *srcdrop,
                   *srcdrop_menu,
                   *sideDrop,
                   *sideDropMenu,
                   *trackDrop,
                   *trackDropMenu,
                   *speedDrop,
                   *speedDrop_Menu,
                   *distdrop,
                   *distdrop_menu,
                   *wpDrop,
                   *wpDropMenu,
                   *fmtdrop,
                   *fmtdrop_menu,
                   *mitem,
                   *frame,
                   *subFrame,
                   *dirBox,
                   *optionBox,
                   *versionlabel,
                   *radio,
                   *check,
                   *textView;
    GSList         *group;

    // process any gtk args
    gtk_init(&argc, &argv);

    // create main window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Heath Imager");

    // handle kills
    gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(quitpressed), NULL);

    // build up the window
    gtk_container_border_width(GTK_CONTAINER(window), 5);
    vbox = gtk_vbox_new(FALSE, 5);

    // selection of fc5025 drive.
    frame = gtk_frame_new("Source Drive");
    gtk_container_border_width(GTK_CONTAINER(frame), 5);
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show(frame);
    srcdrop = gtk_option_menu_new();
    srcdrop_menu = gtk_menu_new();
    add_drives(srcdrop_menu);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(srcdrop), srcdrop_menu);
    gtk_widget_show(srcdrop);
    /* gtk_box_pack_start(GTK_BOX(vbox),srcdrop,FALSE,FALSE,0); */
    gtk_container_add(GTK_CONTAINER(frame), srcdrop);


    

#if 0
    frame = gtk_frame_new("Disk Type");
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show(frame);
    fmtdrop = gtk_option_menu_new();
    fmtdrop_menu = gtk_menu_new();
    //add_formats(fmtdrop_menu);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(fmtdrop), fmtdrop_menu);
    /* gtk_signal_connect(GTK_OBJECT(fmtdrop_menu),"deactivate",GTK_SIGNAL_FUNC(quitpressed),NULL); */
    gtk_widget_show(fmtdrop);
    gtk_container_add(GTK_CONTAINER(frame), fmtdrop);
#endif
    // Browse functionality 
#if 0
    browsebutton = gtk_button_new_with_label("Browse Disk Contents");
    gtk_box_pack_start(GTK_BOX(vbox), browsebutton, FALSE, FALSE, 0);
    //gtk_signal_connect(GTK_OBJECT(browsebutton), "clicked", GTK_SIGNAL_FUNC(browsepressed), NULL);
    gtk_widget_show(browsebutton);
#endif

    frame = gtk_frame_new("Save File");
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show(frame);
    dirBox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(frame), dirBox);
    subFrame = gtk_frame_new("Directory");
    gtk_frame_set_shadow_type(GTK_FRAME(subFrame), GTK_SHADOW_OUT);
    gtk_box_pack_start(GTK_BOX(dirBox), subFrame, FALSE, FALSE, 0);
    gtk_widget_show(subFrame);
    outdir_field = gtk_entry_new();
    populate_outdir();
    gtk_widget_show(outdir_field);
    gtk_container_add(GTK_CONTAINER(subFrame), outdir_field);

    // Recovery Floppy disk
    subFrame = gtk_frame_new("Input Filename");
    gtk_frame_set_shadow_type(GTK_FRAME(subFrame), GTK_SHADOW_OUT);
    gtk_box_pack_start(GTK_BOX(dirBox), subFrame, FALSE, FALSE, 0);
    gtk_widget_show(subFrame);
    in_fname_field = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(in_fname_field), "");
    gtk_widget_show(in_fname_field);
    gtk_container_add(GTK_CONTAINER(subFrame), in_fname_field);
    gtk_widget_show(dirBox);

    // Output disk image
    subFrame = gtk_frame_new("Output Filename");
    gtk_frame_set_shadow_type(GTK_FRAME(subFrame), GTK_SHADOW_OUT);
    gtk_box_pack_start(GTK_BOX(dirBox), subFrame, FALSE, FALSE, 0);
    gtk_widget_show(subFrame);
    fname_field = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(fname_field), "disk0001.h17disk");
    gtk_widget_show(fname_field);
    gtk_container_add(GTK_CONTAINER(subFrame), fname_field);
    gtk_widget_show(dirBox);


    // Disk Format
    frame = gtk_frame_new("Disk Format");
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show(frame);

    optionBox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(frame), optionBox);
    subFrame = gtk_frame_new("Sides");
    gtk_box_pack_start(GTK_BOX(optionBox), subFrame, FALSE, FALSE, 0);
    gtk_widget_show(subFrame); 
   
    sideDrop = gtk_option_menu_new();
    sideDropMenu = gtk_menu_new();
    add_side(sideDropMenu);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(sideDrop), sideDropMenu);
    gtk_container_add(GTK_CONTAINER(subFrame), sideDrop);
    gtk_widget_show(sideDrop);

    subFrame = gtk_frame_new("Tracks");
    gtk_box_pack_start(GTK_BOX(optionBox), subFrame, FALSE, FALSE, 0);
    gtk_widget_show(subFrame);

    trackDrop = gtk_option_menu_new();
    trackDropMenu = gtk_menu_new();
    add_track(trackDropMenu);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(trackDrop), trackDropMenu);
    gtk_container_add(GTK_CONTAINER(subFrame), trackDrop);
    gtk_widget_show(trackDrop);

    // Disk Speed select
    subFrame = gtk_frame_new("Disk Speed (RPM)");
    gtk_box_pack_start(GTK_BOX(optionBox), subFrame, FALSE, FALSE, 0);
    gtk_widget_show(subFrame);

    speedDrop = gtk_option_menu_new();
    speedDrop_Menu = gtk_menu_new();
    add_speed(speedDrop_Menu);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(speedDrop), speedDrop_Menu);
    gtk_container_add(GTK_CONTAINER(subFrame), speedDrop);
    gtk_widget_show(speedDrop);

    gtk_widget_show(optionBox);
 
    // Disk parameters:
    //
    frame = gtk_frame_new("Disk Flags");
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show(frame);
    optionBox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(frame), optionBox);
    subFrame = gtk_frame_new("Distribution Info");
    gtk_box_pack_start(GTK_BOX(optionBox), subFrame, FALSE, FALSE, 0);
    gtk_widget_show(subFrame);

    distdrop = gtk_option_menu_new();
    distdrop_menu = gtk_menu_new();
    add_distribution(distdrop_menu);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(distdrop), distdrop_menu);
    gtk_container_add(GTK_CONTAINER(subFrame), distdrop);
    gtk_widget_show(distdrop);

    subFrame = gtk_frame_new("Disk Write-Protect");
    gtk_box_pack_start(GTK_BOX(optionBox), subFrame, FALSE, FALSE, 0);
    gtk_widget_show(subFrame);

    wpDrop = gtk_option_menu_new();
    wpDropMenu = gtk_menu_new();
    add_wp(wpDropMenu);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(wpDrop), wpDropMenu);
    gtk_container_add(GTK_CONTAINER(subFrame), wpDrop);
    gtk_widget_show(wpDrop);
    

    gtk_widget_show(optionBox);


    // Disk comment:
    frame = gtk_frame_new("Comment");
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show(frame);
    textView = gtk_text_view_new();
    textBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));

    gtk_container_add(GTK_CONTAINER(frame), textView);
    gtk_widget_show(textView);

    //  disk info button
    diskInfoButton = gtk_button_new_with_label("Disk Info");
    gtk_box_pack_start(GTK_BOX(vbox), diskInfoButton, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(diskInfoButton), "clicked", GTK_SIGNAL_FUNC(diskInfoPressed), NULL);
    gtk_widget_show(diskInfoButton);

    //  Image disk button
    imgbutton = gtk_button_new_with_label("Capture Disk Image File");
    gtk_box_pack_start(GTK_BOX(vbox), imgbutton, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(imgbutton), "clicked", GTK_SIGNAL_FUNC(imgPressed), NULL);
    gtk_widget_show(imgbutton);

    // quit button
    quitbutton = gtk_button_new_with_label("Quit");
    gtk_box_pack_start(GTK_BOX(vbox), quitbutton, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(quitbutton), "clicked", GTK_SIGNAL_FUNC(quitpressed), NULL);
    gtk_widget_show(quitbutton);

    // version text
    versionlabel = gtk_label_new(MY_NAME " version " VERSION_STRING);
    gtk_box_pack_start(GTK_BOX(vbox), versionlabel, FALSE, FALSE, 0);
    gtk_widget_show(versionlabel);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    //gtk_widget_show(quitbutton);
    gtk_widget_show(vbox);
    /* gtk_widget_set_usize(window,100,1); */
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 1);
    gtk_widget_show(window);
    update_sensitivity();

    // kick off gtk
    //
    gtk_main();
    return 0;
}


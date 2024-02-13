#include <gtk/gtk.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <ctime>

#include "disk.h"
#include "heath_hs.h"
#include "drive.h"
#include "fc5025.h"
#include "h17disk.h"
#include "disk_util.h"
#include "raw_sector.h"

#define VERSION_STRING "1.2.0"

#define DIRECTORY_SEPARATOR "/"

#define PROG_NAME "HeathImager"

static GtkWidget              *fname_field;
// static GtkWidget              *in_fname_field;
static GtkWidget              *description_field;
static GtkWidget              *outdir_field;
static struct DriveInfo       *selected_drive   = NULL;
static GtkWidget              *captureButton,
                              *diskInfoButton,
                              *testBoardButton,
                              *testInterfaceButton;
GtkWidget                     *listbox,
                              *pop_button;

static int                     errorCount;
static float progress;

static int                     selected_item_type;
static char                   *selected_item_name;
static int                     modal           = 0;
static int                     img_cancelled;
static int                     diskinfoStop;
static int                     testBoardDone;
static int                     testInterfaceDone;


static bool                    wpDisk          = false;
static uint8_t                 dist_status     = 0;

static uint8_t                 drive_tpi       = 96;
static uint16_t                drive_rpm       = 360;
static uint8_t                 drive_sides     = 2;

static uint8_t                 disk_tracks     = 40;
static uint8_t                 disk_sides      = 1;

static GtkTextBuffer          *textBufferLabel;
static GtkTextBuffer          *textBufferComment;
static GtkTextBuffer          *textBufferImager;

//using namespace std;


bool fileExists(char *name)
{
    if (FILE *file = fopen(name, "r"))
    {
        fclose(file);
        return true;
    }
    else
    {
        return false;
    }
}


void
addDateToFile(H17Disk *image)
{
    std::time_t time = std::time(nullptr);
    struct tm *timeInfo;
    char timeString[100];

    timeInfo = gmtime(&time);

    uint32_t length = strftime(timeString, 100, "%c", timeInfo);

    if (length)
    {
        image->writeDate((unsigned char *) timeString, length + 1);
    }
}


void
addProgramToFile(H17Disk *image)
{
    std::string  programName;

    programName = PROG_NAME;
    programName +=  " ";
    programName +=  VERSION_STRING;
    uint32_t length = programName.length();

    if (length)
    {
        image->writeProgram((unsigned char *) programName.c_str(), length + 1);
    }
}


void
addLabelToFile(H17Disk *image)
{
    GtkTextIter    start,
                   end;
    size_t         length;
    gchar         *text;

    gtk_text_buffer_get_bounds(textBufferLabel, &start, &end);

    text = gtk_text_buffer_get_text (textBufferLabel, &start, &end, TRUE);
    length = strlen(text);

    if (length)
    {
        image->writeLabel((unsigned char *)text, (uint32_t) length + 1);
    }
    g_free(text);
}


void
addCommentToFile(H17Disk *image)
{
    GtkTextIter    start,
                   end;
    size_t         length;
    gchar         *text;

    gtk_text_buffer_get_bounds(textBufferComment, &start, &end);

    text = gtk_text_buffer_get_text (textBufferComment, &start, &end, TRUE);
    length = strlen(text);

    if (length)
    {
        image->writeComment((unsigned char *)text, (uint32_t) length + 1);
    }
    g_free(text);
}


void
addImagerToFile(H17Disk *image)
{
    GtkTextIter    start,
                   end;
    size_t         length;
    gchar         *text;

    gtk_text_buffer_get_bounds(textBufferImager, &start, &end);

    text = gtk_text_buffer_get_text (textBufferImager, &start, &end, TRUE);
    length = strlen(text);

    if (length > 1)
    {
        image->writeImager((unsigned char *)text, (uint32_t) length + 1);
    }
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
quitPressed(GtkWidget * widget, gpointer gdata)
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
    GtkWidget   *filew = (GtkWidget *) gdata;

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
    const int      maxRetries_c = 6;
    uint8_t        buf[HeathHSDisk::defaultSectorBytes()];
    uint8_t        rawBuf[HeathHSDisk::defaultSectorRawBytes()];
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
            gtk_label_set_text(GTK_LABEL(error_label), errtext);
            refresh_screen();
        }

        // If it was a read error, then raw bytes are not valid, otherwise store raw
        if (retVal != Err_ReadError)
        {
            //printf("writing raw sector - side: %d track: %d sector: %d\n", side, track, sector);
            image->addRawSector(sector, rawBuf, disk->sectorRawBytes(side, track, sector));
        }
    }
    while ((retVal != 0) && (retryCount++ < maxRetries_c));

    // even if there is an error, use the last processed sector for storage, unless
    // it was an error of type - read error.
    //
    if (retVal == Err_ReadError)
    {
        //printf("read error writing empty raw sector -  side: %d track: %d sector: %d\n", side, track, sector);
        image->addSector(sector, retVal, nullptr, 0);
    }
    else
    {
        //printf("writing good sector - side: %d track: %d sector: %d\n", side, track, sector);
        image->addSector(sector, retVal, buf, disk->sectorBytes(side, track, sector));
    }

    if (retVal != 0)
    {
        errorCount++;
        snprintf(errtext, sizeof(errtext), "Failed after %d attempts: %d H: %d T: %d S:%d",
                 maxRetries_c, retVal, side, track, sector);
        gtk_label_set_text(GTK_LABEL(error_label), errtext);
        refresh_screen();
    }
    else
    {
        snprintf(errtext, sizeof(errtext), "Passed after %d attempts -  H: %d T: %d S:%d",
                 retryCount, side, track, sector);
        gtk_label_set_text(GTK_LABEL(error_label), errtext);
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
    SectorList              *sector_list,
                            *sector_entry;
    int                      num_sectors = disk->numSectors(track, side);
    int                      halfway_mark = num_sectors >> 1;
    std::vector<RawSector *> rawSectors;
    std::vector<Sector *>    sectors;

    if (FC5025::inst()->seek(disk->physicalTrack(track)) != 0)
    {
        gtk_label_set_text(GTK_LABEL(status_label), "Unable to seek to track! Giving up.");
        return 1;
    }
    refresh_screen();

    sector_list = (SectorList *) malloc(sizeof(SectorList) * num_sectors);
    if (!sector_list)
    {
        gtk_label_set_text(GTK_LABEL(status_label), "Out of memory! Giving up.");
        return 1;
    }

    disk->genBestReadOrder(sector_list, track, side);

    sector_entry = sector_list;
    image->startTrack(side, track);

    while (num_sectors--)
    {
        //printf("%s: reading one sector: %d\n", __FUNCTION__, sector_entry->sector);
        read_one_sector(image, disk, track, side, sector_entry->sector, error_label);
        if (img_cancelled)
        {
            gtk_label_set_text(GTK_LABEL(status_label), "Cancelled.");
            free(sector_list);
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
    {
        return;
    }
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
setTestBoardDone(GtkWidget *widget, gpointer gdata)
{
    testBoardDone = 1;
}

void
setTestInterfaceDone(GtkWidget *widget, gpointer gdata)
{
    testInterfaceDone = 1;
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
          GtkWidget * button_label, GtkWidget * button, GtkWidget * cancelButton, char *text)
{
    if (text)
    {
        gtk_label_set_text(GTK_LABEL(status_label), text);
    }
    gtk_label_set_text(GTK_LABEL(button_label), "Bummer.");
    gtk_widget_set_sensitive(cancelButton, 0);
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
configDrivePressed(GtkWidget *widget, gpointer gdata)
{

    GtkWidget      *driveInfoWindow  = gtk_dialog_new();
    GtkWidget      *saveButton       = gtk_button_new_with_label("Save");
    GtkWidget      *cancelButton     = gtk_button_new_with_label("Cancel");
    GtkWidget      *trackLabel       = gtk_label_new("Track:   --\n");
    GtkWidget      *speedLabel       = gtk_label_new("RPM:     --\n");
    GtkWidget      *sectorCountLabel = gtk_label_new("Sectors: --\n");
    GtkWidget      *flagsLabel       = gtk_label_new("Flags:   --\n");
    GtkWidget      *errorLabel       = gtk_label_new("");
    char            status_text[80];
    uint8_t         track,
                    sectorCount,
                    flags;
    uint16_t        speed;
    int             retVal;
    gint            delete_signal;
    Drive           drive(selected_drive);


}

void
diskInfoPressed(GtkWidget * widget, gpointer gdata)
{
    GtkWidget      *diskInfoWindow   = gtk_dialog_new();
    GtkWidget      *stopButton       = gtk_button_new_with_label("Stop");
    GtkWidget      *doneButton       = gtk_button_new_with_label("Done");
    GtkWidget      *trackLabel       = gtk_label_new("Track:   --\n");
    GtkWidget      *speedLabel       = gtk_label_new("RPM:     --\n");
    GtkWidget      *sectorCountLabel = gtk_label_new("Sectors: --\n");
    GtkWidget      *flagsLabel       = gtk_label_new("Flags:   --\n");
    GtkWidget      *errorLabel       = gtk_label_new("");
    char            status_text[80];
    uint8_t         track,
                    sectorCount,
                    flags;
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


    if (drive.getStatus() != 0)
    {
       // gtk_label_set_text(GTK_LABEL(errorLabel), "Unable to open drive");
        return;
    }
    while (!diskinfoStop)
    {
        if ((retVal = FC5025::inst()->driveStatus(&track, &speed, &sectorCount, &flags)) != 0)
        {
            gtk_label_set_text(GTK_LABEL(errorLabel), "Unable to get drive status");
            gtk_widget_set_sensitive(doneButton, 1);
            gtk_widget_set_sensitive(stopButton, 0);
            gtk_signal_disconnect(GTK_OBJECT(diskInfoWindow), delete_signal);
            modal = 0;
            return;
        }
        snprintf(status_text, sizeof(status_text), "Track:        %02d\n", track);
        gtk_label_set_text(GTK_LABEL(trackLabel), status_text);
        snprintf(status_text, sizeof(status_text), "Speed:        %6.2f\n", speed/100.0);
        gtk_label_set_text(GTK_LABEL(speedLabel), status_text);
        snprintf(status_text, sizeof(status_text), "Sector Count: %02d\n", sectorCount);
        gtk_label_set_text(GTK_LABEL(sectorCountLabel), status_text);
        snprintf(status_text, sizeof(status_text), "Flags:        0x%02x\n", flags);
        gtk_label_set_text(GTK_LABEL(flagsLabel), status_text);
        refresh_screen();
        usleep(100000);
    }
    gtk_widget_set_sensitive(doneButton, 1);
    gtk_widget_set_sensitive(stopButton, 0);
    gtk_signal_disconnect(GTK_OBJECT(diskInfoWindow), delete_signal);
    modal = 0;
}

void
testBoardPressed(GtkWidget * widget, gpointer gdata)
{
    GtkWidget      *testBoardWindow  = gtk_dialog_new();
    GtkWidget      *okButton         = gtk_button_new_with_label("Ok");

    GtkWidget      *resultLabel      = gtk_label_new("Result:  ----\n");
    char            status_text[80];
    int             retVal;
    gint            delete_signal;
    Drive           drive(selected_drive);

    testBoardDone = 0;

    gtk_window_set_title(GTK_WINDOW(testBoardWindow), "Board Test");
    delete_signal = gtk_signal_connect(GTK_OBJECT(testBoardWindow), "delete_event",
                                       GTK_SIGNAL_FUNC(disallow_delete), NULL);
    gtk_signal_connect(GTK_OBJECT(testBoardWindow), "destroy", (GtkSignalFunc) destroy_diskinfo,
                       (gpointer) testBoardWindow);
    modal = 1;
    gtk_container_border_width(GTK_CONTAINER(testBoardWindow), 10);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(testBoardWindow)->vbox), resultLabel, TRUE, TRUE, 0);
    gtk_widget_show(resultLabel);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(testBoardWindow)->action_area), okButton, TRUE,
                       TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(okButton), "clicked", GTK_SIGNAL_FUNC(setTestBoardDone),
                       testBoardWindow);
    gtk_widget_set_sensitive(okButton, 1);
    gtk_widget_show(okButton);


    if (drive.getStatus() != 0)
    {
       // gtk_label_set_text(GTK_LABEL(errorLabel), "Unable to open drive");
        return;
    }

    while (!testBoardDone)
    {
    /*
        if ((retVal = FC5025::inst()->driveStatus(&track, &speed, &sectorCount, &flags)) != 0)
        {
            gtk_label_set_text(GTK_LABEL(errorLabel), "Unable to get drive status");
            gtk_widget_set_sensitive(doneButton, 1);
            gtk_widget_set_sensitive(stopButton, 0);
            gtk_signal_disconnect(GTK_OBJECT(diskInfoWindow), delete_signal);
            modal = 0;
            return;
        }
        snprintf(status_text, sizeof(status_text), "Track:        %02d\n", track);
        gtk_label_set_text(GTK_LABEL(trackLabel), status_text);
        snprintf(status_text, sizeof(status_text), "Speed:        %6.2f\n", speed/100.0);
        gtk_label_set_text(GTK_LABEL(speedLabel), status_text);
        snprintf(status_text, sizeof(status_text), "Sector Count: %02d\n", sectorCount);
        gtk_label_set_text(GTK_LABEL(sectorCountLabel), status_text);
        snprintf(status_text, sizeof(status_text), "Flags:        0x%02x\n", flags);
        gtk_label_set_text(GTK_LABEL(flagsLabel), status_text);
        refresh_screen();
        usleep(100000);
    */
    }
    gtk_signal_disconnect(GTK_OBJECT(testBoardWindow), delete_signal);
    modal = 0;
}

/// Start imaging a disk.
void
capturePressed(GtkWidget * widget, gpointer gdata)
{
    GtkWidget      *image_window = gtk_dialog_new();
    GtkAdjustment  *adj          = (GtkAdjustment *) gtk_adjustment_new(0, 0, 400, 0, 0, 0);
    GtkWidget      *progressbar  = gtk_progress_bar_new_with_adjustment(adj);
    GtkWidget      *button       = gtk_button_new();
    GtkWidget      *cancelButton = gtk_button_new_with_label("Cancel");
    char            status_text[80];
    GtkWidget      *status_label = gtk_label_new("Preparing...");
    GtkWidget      *error_label  = gtk_label_new("");
    GtkWidget      *button_label = gtk_label_new("In progress...");
    float           progress_per_halftrack;
    Disk           *disk         = new HeathHSDisk(disk_sides, disk_tracks, drive_tpi, drive_rpm);
    int             track,
                    side;
    //char           *in_filename;
    char           *out_filename;
    gint            delete_signal;
    H17Disk        *image;
    Drive           drive(selected_drive);
    // TODO have a way to restart a capture without starting from scratch, should read
    // in the existing image file, and determine which sectors are good/bad and re-read the
    // bad ones
    //bool            recovery;

    progress = 0;
    errorCount = 0;

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

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(image_window)->action_area), cancelButton, TRUE,
                       TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(cancelButton), "clicked", GTK_SIGNAL_FUNC(imgcancel),
                       image_window);
    gtk_widget_set_sensitive(cancelButton, 0);
    gtk_widget_show(cancelButton);

    gtk_container_add(GTK_CONTAINER(button), button_label);
    gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(imgdone), image_window);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(image_window)->action_area), button, TRUE, TRUE, 0);
    gtk_widget_set_sensitive(button, 0);
    gtk_widget_show(button_label);
    gtk_widget_show(button);

    gtk_widget_show(image_window);
    gtk_grab_add(image_window);
    refresh_screen();

    if (drive.getStatus() != 0)
    {
        imgFailed(image_window, delete_signal, status_label, button_label,
                  button, cancelButton, (char *) "Unable to open drive.");
        return;
    }
    refresh_screen();

    // if (strlen(gtk_entry_get_text(GTK_ENTRY(in_fname_field))) != 0)
    // {
    //     recovery = true;
    // }
    // if (recovery)
    // {
    //     in_filename = (char *) malloc(strlen(gtk_entry_get_text(GTK_ENTRY(outdir_field))) + 1 +
    //                           strlen(gtk_entry_get_text(GTK_ENTRY(in_fname_field))) + 1);
    //     if (!in_filename)
    //     {
    //         imgFailed(image_window, delete_signal, status_label, button_label,
    //                   button, cancelButton, (char *) "Out of memory!");
    //         return;
    //     }
    //     strcpy(in_filename, gtk_entry_get_text(GTK_ENTRY(outdir_field)));
    //     strcat(in_filename, DIRECTORY_SEPARATOR);
    //     strcat(in_filename, gtk_entry_get_text(GTK_ENTRY(in_fname_field)));
    // }

    out_filename = (char *) malloc(strlen(gtk_entry_get_text(GTK_ENTRY(outdir_field))) + 1 +
                          strlen(gtk_entry_get_text(GTK_ENTRY(fname_field))) + 1);
    if (!out_filename)
    {
        imgFailed(image_window, delete_signal, status_label, button_label,
                  button, cancelButton, (char *) "Out of memory!");
        return;
    }
    strcpy(out_filename, gtk_entry_get_text(GTK_ENTRY(outdir_field)));
    strcat(out_filename, DIRECTORY_SEPARATOR);
    strcat(out_filename, gtk_entry_get_text(GTK_ENTRY(fname_field)));

    if (fileExists(out_filename))
    {
        imgFailed(image_window, delete_signal, status_label, button_label,
                  button, cancelButton, (char *) "File already exists!");
        return;
    }
    image = new H17Disk();
    // if (recovery)
    // {
    //    image->openForRecovery(in_filename);
    // }

    if (!image->openForWrite(out_filename))
    {
        imgFailed(image_window, delete_signal, status_label, button_label,
                  button, cancelButton, (char *) "File can not be opened!");
        return;
    }

    if (FC5025::inst()->recalibrate() != 0)
    {
        image->closeFile();
        imgFailed(image_window, delete_signal, status_label, button_label,
                  button, cancelButton, (char *) "Unable to recalibrate drive.");
        return;
    }
    refresh_screen();

    if (FC5025::inst()->setDensity(disk->density()) != 0)
    {
        image->closeFile();
        imgFailed(image_window, delete_signal, status_label, button_label,
                  button, cancelButton, (char *) "Unable to set density.");
        return;
    }
    refresh_screen();

    // print file header for this disk
    //
    image->writeHeader();
    image->setSides(disk->numSides());
    image->setTracks(disk->numTracks());
    image->writeDiskFormatBlock();

    image->setWPParameter(wpDisk);
    image->setDistributionParameter(dist_status);
    image->setTrackDataParameter(3);
    image->writeParameters();
    addLabelToFile(image);
    addCommentToFile(image);
    addDateToFile(image);
    addImagerToFile(image);
    addProgramToFile(image);
    image->startData();

    img_cancelled = 0;
    gtk_widget_set_sensitive(cancelButton, 1);
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
            gtk_label_set_text(GTK_LABEL(status_label), status_text);
            refresh_screen();

            // image the track
            if (image_track(image, disk, track, side, status_label, error_label, progressbar,
                            progress_per_halftrack) != 0)
            {
                image->endDataBlock();
                image->writeRawDataBlock();
                image->closeFile();
                imgFailed(image_window, delete_signal, status_label, button_label,
                          button, cancelButton, NULL);
                return;
            }
        }
    }
    image->endDataBlock();
    image->writeRawDataBlock();

    image->closeFile();

    // commenting this out for now, when the head is left at a
    // high sector, it's easier to clean head with q-tip. 
    //FC5025::inst()->seek(0);

    if (!errorCount)
    {
        gtk_label_set_text(GTK_LABEL(status_label), "Successfully read disk.");
        gtk_label_set_text(GTK_LABEL(button_label), "Yay!");
    }
    else
    {
        char statusText[60];
        snprintf(statusText, sizeof(statusText), "Total of %d sectors had errors\n", errorCount);

        gtk_label_set_text(GTK_LABEL(status_label), statusText);
        gtk_label_set_text(GTK_LABEL(button_label), "Bummer.");
    }

    gtk_widget_set_sensitive(cancelButton, 0);
    gtk_widget_set_sensitive(button, 1);
    gtk_signal_disconnect(GTK_OBJECT(image_window), delete_signal);
    modal = 0;
}


void
update_sensitivity(void)
{
    if (selected_drive == NULL)
    {
        gtk_widget_set_sensitive(captureButton, 0);
        gtk_widget_set_sensitive(diskInfoButton, 0);
        //gtk_widget_set_sensitive(testBoardButton, 0);
        //gtk_widget_set_sensitive(testInterfaceButton, 0);
    }
    else
    {
        gtk_widget_set_sensitive(captureButton, 1);
        gtk_widget_set_sensitive(diskInfoButton, 1);
        //gtk_widget_set_sensitive(testBoardButton, 1);
        //gtk_widget_set_sensitive(testInterfaceButton, 1);
    }
}


void
drive_changed(GtkWidget * widget, gpointer data)
{
    selected_drive = (DriveInfo *) data;
    update_sensitivity();
}

// Disk Flags
void
dist_changed(GtkWidget *widget, gpointer data)
{
    dist_status = *(uint8_t *) data;
}


void
wp_changed(GtkWidget *widget, gpointer data)
{
    wpDisk = *(bool *) data;
}

// disk settings
void
disk_side_changed(GtkWidget *widget, gpointer data)
{
    disk_sides = *(uint8_t *) data;
}


void
disk_track_changed(GtkWidget *widget, gpointer data)
{
    disk_tracks = *(uint8_t *) data;
}

void
add_disk_track(GtkWidget *menu)
{
    GtkWidget       *mitem;
    static uint8_t   value[2] = {40, 80};

    mitem = gtk_menu_item_new_with_label("40 Tracks");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate",
                       GTK_SIGNAL_FUNC(disk_track_changed), &value[0]);

    mitem = gtk_menu_item_new_with_label("80 Tracks");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate",
                       GTK_SIGNAL_FUNC(disk_track_changed), &value[1]);
}


void
add_disk_side(GtkWidget *menu)
{
    GtkWidget       *mitem;
    static uint8_t   value[2] = {1, 2};

    mitem = gtk_menu_item_new_with_label("Single-Sided");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate",
                       GTK_SIGNAL_FUNC(disk_side_changed), &value[0]);

    mitem = gtk_menu_item_new_with_label("Double-Sided");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate",
                       GTK_SIGNAL_FUNC(disk_side_changed), &value[1]);
}


void
drive_side_changed(GtkWidget *widget, gpointer data)
{
    printf("side_changed: %d\n",*(uint8_t *)  data);
    drive_sides = *(uint8_t *) data;
}


void
add_drive_sides(GtkWidget *menu)
{
    GtkWidget       *mitem;
    static uint8_t   value[2] = {1, 2};

    mitem = gtk_menu_item_new_with_label("Single-Sided");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate",
                       GTK_SIGNAL_FUNC(drive_side_changed), &value[0]);

    mitem = gtk_menu_item_new_with_label("Double-Sided");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate",
                       GTK_SIGNAL_FUNC(drive_side_changed), &value[1]);

    gtk_menu_set_active(GTK_MENU(menu), 1);
}

void
drive_tpi_changed(GtkWidget *widget, gpointer data)
{
    drive_tpi = *(uint8_t *) data;
    // tpi_status = (track_status == 80) ? 96 : 48;
}

void
add_drive_tpi(GtkWidget *menu)
{
    GtkWidget       *mitem;
    static uint8_t   value[2] = {48, 96};

    mitem = gtk_menu_item_new_with_label( "40 Track/48 TPI");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate",
                       GTK_SIGNAL_FUNC(drive_tpi_changed), &value[0]);

    mitem = gtk_menu_item_new_with_label( "80 Track/96 TPI");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate",
                       GTK_SIGNAL_FUNC(drive_tpi_changed), &value[1]);

    gtk_menu_set_active(GTK_MENU(menu), 1);
}


void
drive_rpm_changed(GtkWidget *widget, gpointer data)
{
    printf("original rpm: %d\n", drive_rpm);
    drive_rpm = *(int *) data;
    printf("new rpm: %d\n", drive_rpm);
}


void
add_drive_rpm(GtkWidget *menu)
{
    GtkWidget      *mitem;
    static int   value[2] = {300, 360};

    mitem = gtk_menu_item_new_with_label( "300 RPM");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate", GTK_SIGNAL_FUNC(drive_rpm_changed), &value[0]);
    gtk_menu_item_deselect((GtkMenuItem *) mitem);

    mitem = gtk_menu_item_new_with_label( "360 RPM");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate", GTK_SIGNAL_FUNC(drive_rpm_changed), &value[1]);
    gtk_menu_item_select((GtkMenuItem *) mitem);

    gtk_menu_set_active(GTK_MENU(menu), 1);
}


void
add_wp(GtkWidget *menu)
{
    GtkWidget      *mitem;
    static bool     value[2] = {false, true};

    mitem = gtk_menu_item_new_with_label( "Write Enabled");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate", GTK_SIGNAL_FUNC(wp_changed), &value[0]);

    mitem = gtk_menu_item_new_with_label( "Write Protected");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate", GTK_SIGNAL_FUNC(wp_changed), &value[1]);
}


void
add_distribution(GtkWidget *menu)
{
    GtkWidget      *mitem;
    static uint8_t  value[4] = {0, 1, 2, 3 };

    mitem = gtk_menu_item_new_with_label( "Disk Status Unknown");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate", GTK_SIGNAL_FUNC(dist_changed), &value[0]);

    mitem = gtk_menu_item_new_with_label( "Distribution Disk");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate", GTK_SIGNAL_FUNC(dist_changed), &value[1]);

    mitem = gtk_menu_item_new_with_label( "User Disk");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate", GTK_SIGNAL_FUNC(dist_changed), &value[2]);

    mitem = gtk_menu_item_new_with_label( "Copy of a Distribution Disk");
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_show(mitem);
    gtk_signal_connect(GTK_OBJECT(mitem), "activate", GTK_SIGNAL_FUNC(dist_changed), &value[3]);
}


void
add_drives(GtkWidget * menu)
{
    GtkWidget        *mitem;
    struct DriveInfo *drive,
                     *drives;
    char              label[100];

    drives = Drive::get_drive_list();
    if (!drives)
    {
        mitem = gtk_menu_item_new_with_label( "No drives found.");
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
        mitem = gtk_menu_item_new_with_label(label);
        gtk_menu_append(GTK_MENU(menu), mitem);
        gtk_widget_show(mitem);
        gtk_signal_connect(GTK_OBJECT(mitem), "activate",  GTK_SIGNAL_FUNC(drive_changed), drive);
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
                   *quitButton,
                   *srcdrop,
                   *srcdrop_menu,
                   *sideDrop,
                   *sideDropMenu,
                   *trackDrop,
                   *trackDropMenu,
                   *speedDrop,
                   *speedDrop_Menu,
                   *driveSidesDrop,
                   *driveSidesDrop_Menu,
                   *driveTpiDrop,
                   *driveTpiDrop_Menu,
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
                   *textViewLabel,
                   *textViewComment,
                   *textViewImager;
    GSList         *group;

    // process any gtk args
    gtk_init(&argc, &argv);

    // create main window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Heath Imager");

    // handle kills
    gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(quitPressed), NULL);

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
    gtk_container_add(GTK_CONTAINER(frame), srcdrop);


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
    // subFrame = gtk_frame_new("Input Filename");
    // gtk_frame_set_shadow_type(GTK_FRAME(subFrame), GTK_SHADOW_OUT);
    // gtk_box_pack_start(GTK_BOX(dirBox), subFrame, FALSE, FALSE, 0);
    // gtk_widget_show(subFrame);
    // in_fname_field = gtk_entry_new();
    // gtk_entry_set_text(GTK_ENTRY(in_fname_field), "");
    // gtk_widget_show(in_fname_field);
    // gtk_container_add(GTK_CONTAINER(subFrame), in_fname_field);
    // gtk_widget_show(dirBox);

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

    // disk sides
    subFrame = gtk_frame_new("Sides");
    gtk_box_pack_start(GTK_BOX(optionBox), subFrame, FALSE, FALSE, 0);
    gtk_widget_show(subFrame);

    sideDrop = gtk_option_menu_new();
    sideDropMenu = gtk_menu_new();
    add_disk_side(sideDropMenu);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(sideDrop), sideDropMenu);
    gtk_container_add(GTK_CONTAINER(subFrame), sideDrop);
    gtk_widget_show(sideDrop);

    // disk Tracks
    subFrame = gtk_frame_new("Tracks");
    gtk_box_pack_start(GTK_BOX(optionBox), subFrame, FALSE, FALSE, 0);
    gtk_widget_show(subFrame);

    trackDrop = gtk_option_menu_new();
    trackDropMenu = gtk_menu_new();
    add_disk_track(trackDropMenu);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(trackDrop), trackDropMenu);
    gtk_container_add(GTK_CONTAINER(subFrame), trackDrop);
    gtk_widget_show(trackDrop);
    gtk_widget_show(optionBox);

    // Disk Speed select - not needed all hard-sectored disks are 300 RPM and only the drive changes between 300 or 360

    // Drive settings
    frame = gtk_frame_new("Drive Settings");
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 5);
    gtk_widget_show(frame);
    optionBox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(frame), optionBox);

    // Drive RPM
    subFrame = gtk_frame_new("Speed (RPM)");
    gtk_box_pack_start(GTK_BOX(optionBox), subFrame, FALSE, FALSE, 5);
    gtk_widget_show(subFrame);

    speedDrop = gtk_option_menu_new();
    speedDrop_Menu = gtk_menu_new();
    add_drive_rpm(speedDrop_Menu);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(speedDrop), speedDrop_Menu);
    gtk_container_add(GTK_CONTAINER(subFrame), speedDrop);
    gtk_widget_show(speedDrop);

    // Drive Sides
    subFrame = gtk_frame_new("Sides");
    gtk_box_pack_start(GTK_BOX(optionBox), subFrame, FALSE, FALSE, 5);
    gtk_widget_show(subFrame);

    driveSidesDrop = gtk_option_menu_new();
    //driveSidesDrop = gtk_combo_box_new();
    driveSidesDrop_Menu = gtk_menu_new();
    add_drive_sides(driveSidesDrop_Menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(driveSidesDrop), 1);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(driveSidesDrop), driveSidesDrop_Menu);
    gtk_container_add(GTK_CONTAINER(subFrame), driveSidesDrop);
    gtk_widget_show(driveSidesDrop);

    // Drive TPI
    subFrame = gtk_frame_new("TPI");
    gtk_box_pack_start(GTK_BOX(optionBox), subFrame, FALSE, FALSE, 5);
    gtk_widget_show(subFrame);

    driveTpiDrop = gtk_option_menu_new();
    driveTpiDrop_Menu = gtk_menu_new();
    add_drive_tpi(driveTpiDrop_Menu);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(driveTpiDrop), driveTpiDrop_Menu);
    gtk_container_add(GTK_CONTAINER(subFrame), driveTpiDrop);
    gtk_widget_show(driveTpiDrop);


    gtk_widget_show(optionBox);

    // Disk parameters:
    //
    frame = gtk_frame_new("Disk Flags");
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 20);
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

    // Disk label:
    frame = gtk_frame_new("Label");
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show(frame);
    textViewLabel = gtk_text_view_new();
    textBufferLabel = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textViewLabel));
    gtk_container_add(GTK_CONTAINER(frame), textViewLabel);
    gtk_widget_show(textViewLabel);

    // Disk comment:
    frame = gtk_frame_new("Comment");
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show(frame);
    textViewComment = gtk_text_view_new();
    textBufferComment = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textViewComment));
    gtk_container_add(GTK_CONTAINER(frame), textViewComment);
    gtk_widget_show(textViewComment);

    // Disk imager:
    frame = gtk_frame_new("Imager");
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show(frame);
    textViewImager = gtk_text_view_new();
    textBufferImager = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textViewImager));

    gtk_container_add(GTK_CONTAINER(frame), textViewImager);
    gtk_widget_show(textViewImager);

    //  disk info button
    diskInfoButton = gtk_button_new_with_label("Disk Info");
    gtk_box_pack_start(GTK_BOX(vbox), diskInfoButton, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(diskInfoButton), "clicked", GTK_SIGNAL_FUNC(diskInfoPressed), NULL);
    gtk_widget_show(diskInfoButton);
/*
    //  Test Board button
    testBoardButton = gtk_button_new_with_label("Test Board");
    gtk_box_pack_start(GTK_BOX(vbox), testBoardButton, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(testBoardButton), "clicked", GTK_SIGNAL_FUNC(testBoardPressed), NULL);
    gtk_widget_show(testBoardButton);

    //  Test Interface button
    testInterfaceButton = gtk_button_new_with_label("Test Interface");
    gtk_box_pack_start(GTK_BOX(vbox), testInterfaceButton, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(testInterfaceButton), "clicked", GTK_SIGNAL_FUNC(testInterfacePressed), NULL);
    gtk_widget_show(testInterfaceButton);
*/
    //  capture Image disk button
    captureButton = gtk_button_new_with_label("Capture Disk Image File");
    gtk_box_pack_start(GTK_BOX(vbox), captureButton, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(captureButton), "clicked", GTK_SIGNAL_FUNC(capturePressed), NULL);
    gtk_widget_show(captureButton);

    // quit button
    quitButton = gtk_button_new_with_label("Quit");
    gtk_box_pack_start(GTK_BOX(vbox), quitButton, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(quitButton), "clicked", GTK_SIGNAL_FUNC(quitPressed), NULL);
    gtk_widget_show(quitButton);

    // version text
    versionlabel = gtk_label_new(PROG_NAME " version " VERSION_STRING);
    gtk_box_pack_start(GTK_BOX(vbox), versionlabel, FALSE, FALSE, 0);
    gtk_widget_show(versionlabel);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show(vbox);
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    gtk_widget_show(window);
    update_sensitivity();

    // kick off gtk
    //
    gtk_main();

    return 0;
}


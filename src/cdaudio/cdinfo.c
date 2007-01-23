/*
 *  cdinfo.c   Copyright 1999 Espen Skoglund <esk@ira.uka.de>
 *             Copyright 1999 H�vard Kv�len <havardk@sol.no>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "cdinfo.h"

#include <glib.h>
#include <audacious/i18n.h>
#include <glib/gprintf.h>

#include <audacious/rcfile.h>

#include "cdaudio.h"


/*
 * Function cdda_cdinfo_flush (cdinfo)
 *
 *    Free all information stored about the CD.
 *
 */
void
cdda_cdinfo_flush(cdinfo_t * cdinfo)
{
    trackinfo_t *t;
    gint i;

    if (cdinfo->albname)
        g_free(cdinfo->albname);
    if (cdinfo->artname)
        g_free(cdinfo->artname);

    cdinfo->albname = cdinfo->artname = NULL;

    for (t = cdinfo->tracks, i = 0; i < 100; i++, t++) {
        if (t->artist)
            g_free(t->artist);
        if (t->title)
            g_free(t->title);

        t->artist = t->title = NULL;
        t->num = -1;
    }
    cdinfo->is_valid = FALSE;
}


/*
 * Function cdda_cdinfo_delete (cdinfo)
 *
 *    Free the indicated `cdinfo' structure.
 *
 */
void
cdda_cdinfo_delete(cdinfo_t * cdinfo)
{
    cdda_cdinfo_flush(cdinfo);
    g_free(cdinfo);
}


/*
 * Function cdda_cdinfo_new ()
 *
 *    Allocate a new `cdinfo' structure and return it.
 *
 */
cdinfo_t *
cdda_cdinfo_new(void)
{
    cdinfo_t *ret;
    ret = g_malloc0(sizeof(cdinfo_t));
    cdda_cdinfo_flush(ret);

    return ret;
}


/*
 * Function cdda_cdinfo_track_set (cdinfo, num, artist, title)
 *
 *    Set `artist', and `title' for a track `num'.  If the CD is a
 *    singleartist disc, the `artist' on each track should be set to
 *    NULL.
 *
 */
void
cdda_cdinfo_track_set(cdinfo_t * cdinfo, gint num, gchar * artist,
                      gchar * title)
{
    trackinfo_t *track = cdinfo->tracks + num;

    /* Check bounds */
    if (num < 1 || num >= 100)
        return;

    track->artist = artist;
    track->title = title;
    track->num = num;
    cdinfo->is_valid = TRUE;
}


/*
 * Function cdda_cdinfo_cd_set (cdinfo, cdname, cdartist, discid,
 * genre, year)
 *
 *    Set name, artist, discid, genre and year for a cd. If CD is a
 *    multiartist disc, the `artist' should be set to NULL.
 *
 */
void
cdda_cdinfo_cd_set(cdinfo_t * cdinfo, gchar * cdname, gchar * cdartist,
                   gchar * discid, gchar * genre, gchar *year)
{
    cdinfo->is_valid = TRUE;
    cdinfo->albname = cdname;
    cdinfo->artname = cdartist;
    cdinfo->discid = discid ? atoi(discid) : 0;
    cdinfo->genre = genre;
    cdinfo->year = year ? atoi(year) : 0;
}


/*
 * Function cdda_cdinfo_get (cdinfo, num, artist, album, title)
 *
 *    Get artist, album, and title of the indicated track (i.e. store
 *    them in the specified pointers).  Return 0 upon success, or -1
 *    of track did not exist.  The returned name must be subsequently
 *    freed using g_free().
 *
 */
gint
cdda_cdinfo_get(cdinfo_t * cdinfo, gint num, gchar ** artist,
                gchar ** album, gchar ** title)
{
    trackinfo_t *track = cdinfo->tracks + num;

    /* Check validity */
    if (!cdinfo->is_valid || num < 1 || num >= 100)
        return -1;

    *artist = track->artist ? track->artist :
        cdinfo->artname ? cdinfo->artname : _("(unknown)");
    *album = cdinfo->albname ? cdinfo->albname : _("(unknown)");
    *title = track->title ? track->title : _("(unknown)");

    return track->num == -1 ? -1 : 0;
}


/*
 * Function cdda_cdinfo_write_file
 *
 * Writes the cdinfo_t structure to disk.
 */


void
cdda_cdinfo_write_file(guint32 cddb_discid, cdinfo_t * cdinfo)
{
    gchar *filename;
    RcFile *rcfile;
    gchar sectionname[10], trackstr[16];
    gint i, numtracks = cddb_discid & 0xff;

    sprintf(sectionname, "%08x", cddb_discid);

    filename =
        g_strconcat(g_get_home_dir(), "/", BMP_RCPATH, "/cdinfo", NULL);
    if ((rcfile = bmp_rcfile_open(filename)) == NULL)
        rcfile = bmp_rcfile_new();

    if (cdinfo->albname)
        bmp_rcfile_write_string(rcfile, sectionname, "Albumname",
                                cdinfo->albname);
    else
        bmp_rcfile_write_string(rcfile, sectionname, "Albumname", "");

    if (cdinfo->artname)
        bmp_rcfile_write_string(rcfile, sectionname, "Artistname",
                                cdinfo->artname);
    if (cdinfo->year) {
        gchar *yearstr = g_strdup_printf("%4d", cdinfo->year);
        bmp_rcfile_write_string(rcfile, sectionname, "Year", yearstr);
        g_free(yearstr);
    }

    if (cdinfo->genre)
        bmp_rcfile_write_string(rcfile, sectionname, "Genre", cdinfo->genre);


    for (i = 1; i <= numtracks; i++) {
        if (cdinfo->tracks[i].artist) {
            sprintf(trackstr, "track_artist%d", i);
            bmp_rcfile_write_string(rcfile, sectionname, trackstr,
                                    cdinfo->tracks[i].artist);
        }
        if (cdinfo->tracks[i].title) {
            sprintf(trackstr, "track_title%d", i);
            bmp_rcfile_write_string(rcfile, sectionname, trackstr,
                                    cdinfo->tracks[i].title);
        }
    }
    bmp_rcfile_write(rcfile, filename);
    bmp_rcfile_free(rcfile);
    g_free(filename);
}

/*
 * Function cdda_cdinfo_read_file
 *
 * Tries to find and read a album from the disk-cache.
 *
 * Returns true if the album is found.
 */

gboolean
cdda_cdinfo_read_file(guint32 cddb_discid, cdinfo_t * cdinfo)
{
    gchar *filename;
    RcFile *rcfile;
    gchar sectionname[10], trackstr[16];
    gint i, numtracks = cddb_discid & 0xff;
    gboolean track_found;
    gchar *yearstr = NULL;

    sprintf(sectionname, "%08x", cddb_discid);

    filename =
        g_strconcat(g_get_home_dir(), "/", BMP_RCPATH, "/cdinfo", NULL);
    if ((rcfile = bmp_rcfile_open(filename)) == NULL) {
        g_free(filename);
        return FALSE;
    }
    g_free(filename);

    if (!bmp_rcfile_read_string
        (rcfile, sectionname, "Albumname", &cdinfo->albname))
        return FALSE;

    bmp_rcfile_read_string(rcfile, sectionname, "Artistname",
                           &cdinfo->artname);

    bmp_rcfile_read_string(rcfile, sectionname, "Year", &yearstr);
    if (yearstr) {
        cdinfo->year = atoi(yearstr);
	g_free(yearstr);
	yearstr = NULL;
    }

    bmp_rcfile_read_string(rcfile, sectionname, "Genre", &cdinfo->genre);

    for (i = 1; i <= numtracks; i++) {
        track_found = FALSE;
        sprintf(trackstr, "track_artist%d", i);
        if (bmp_rcfile_read_string
            (rcfile, sectionname, trackstr, &cdinfo->tracks[i].artist))
            track_found = TRUE;
        sprintf(trackstr, "track_title%d", i);
        if (bmp_rcfile_read_string
            (rcfile, sectionname, trackstr, &cdinfo->tracks[i].title))
            track_found = TRUE;
        if (track_found)
            cdinfo->tracks[i].num = i;
    }
    cdinfo->is_valid = TRUE;
    bmp_rcfile_free(rcfile);
    return TRUE;
}

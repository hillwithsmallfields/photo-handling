/*
 * rmphotos.c
 *
 * Delete photos according to the time they were taken.
 *
 * This is based on an example program placed into the public domain
 * by Dan Fandrich
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <dirent.h>
#include <libexif/exif-data.h>

/* Remove spaces on the right of the string */
static void trim_spaces(char *buf)
{
    char *s = buf-1;
    for (; *buf; ++buf) {
        if (*buf != ' ')
            s = buf;
    }
    *++s = 0; /* nul terminate the string on the first of the final spaces */
}

/* Show the tag name and contents if the tag exists */

static const char *short_options = "cdg:hlm:nqt:vx:";

struct option long_options_data[] = {
  {"dryrun", no_argument, 0, 'n'},
  {"extension", required_argument, 0, 'x'},
  {"gps", no_argument, 0, 'g'},
  {"help", no_argument, 0, 'h'},
  {"hms", required_argument, 0, 'm'},
  {"quiet", no_argument, 0, 'q'},
  {"time", required_argument, 0, 't'},
  {"verbose", no_argument, 0, 'v'},
  {0, 0, 0, 0}
};

char *extension = ".jpg";
char hms = '_';
char time_separator = 'T';
int gps = 0;
char *target_dir = NULL;
int dry_run = 0;
int verbose = 0;

struct option *long_options = long_options_data;

ExifTag tags_to_try[] = {
  EXIF_TAG_DATE_TIME_ORIGINAL,
  EXIF_TAG_DATE_TIME,
  EXIF_TAG_DATE_TIME_DIGITIZED,
  EXIF_TAG_SUB_SEC_TIME,
  EXIF_TAG_SUB_SEC_TIME_ORIGINAL,
  EXIF_TAG_SUB_SEC_TIME_DIGITIZED,
  0
};

static void
print_usage()
{
  printf("Usage:\n  rmphotos [options] filenames\n");
  printf("   Options are:\n");
  printf("     -t --time <char>       Use <char> to separate date and time\n");
  printf("     -m --hms <char>        Use <char> to separate hours, minutes, seconds\n");
  printf("     -g --gps               Build the GPS data into the filename\n");
  printf("     -x --extension <extn>  Use <extn> as the filename extension\n");
  printf("                            Include the '.' explicitly if wanted\n");
  printf("     -l --link              Link instead of rename\n");
  printf("     -d --dir <directory>   Work in <directory>\n");
  printf("     -n --dryrun            Don't actually rename anything\n");
  printf("     -q --quiet             Don't output anything\n");
  printf("     -v --verbose           Show renames\n");
  printf("    \n");
}

int main(int argc, char **argv)
{
  ExifData *ed;
  ExifEntry *entry;
  char *directory_name = ".";
  DIR *dir = opendir(directory_name);
  struct dirent *dirent;
  char **deletion_v;
  unsigned int deletion_c = 0;

  while (1) {
    int option_index = 0;
    char opt = getopt_long(argc, argv,
			   short_options, long_options,
			   &option_index);

    // fprintf(stderr, "Got opt %c\n", opt);

    if (opt == -1) {
      break;
    }

    switch (opt) {
    case 'h':
      print_usage();
      exit(0);
    case 'n':
      dry_run = 1;
      break;
    case 'v':
      verbose = 1;
      break;
    case 'q':
      ed_script = 0;
      verbose = 0;
      break;
    case 'g':
      gps = 1;
      break;
    case 'd':
      directory_name = optarg;
      break;
    case 'x':
      extension = optarg;
      break;
    case 'm':
      hms = optarg[0];
      break;
    case 't':
      time_separator = optarg[0];
      break;
    }
  }

  if (argc < 2) {
    print_usage();
    exit(1);
  }

  deletion_v = (char**)malloc((argc + 1) * sizeof(char*));

  for (; optind < argc; optind++) {
    char *file_date_name = argv[optind];
    fprintf(stderr, "Noting %s for deletion of matching entries\n", file_date_name);
    deletion_v[deletion_c++] = file_date_name;
  }

  if (dir == NULL) {
    fprintf(stderr, "Could not open directory %s\n", directory_name);
    exit(1);
  }

  for (dirent = readdir(dir); dirent != NULL; dirent = readdir(dir)) {
    file_old_name = dirent->d_name;
    fprintf(stderr, "Looking at file %s\n", file_old_name);

    ExifTag try_tag, *try_tag_p;

    char date_string_buf[1024];
    char file_new_name[1024];
    int remaining = 1023;

    /* Load an ExifData object from an EXIF file */
    ed = exif_data_new_from_file(file_old_name);
    if (!ed) {
      printf("File not readable or no EXIF data in file %s\n", file_old_name);
      continue;
    }

    if (gps) {
      ExifEntry *latitude_ref_entry = exif_content_get_entry(ed->ifd[EXIF_IFD_0], EXIF_TAG_GPS_LATITUDE_REF);
      ExifEntry *latitude_entry = exif_content_get_entry(ed->ifd[EXIF_IFD_0], EXIF_TAG_GPS_LATITUDE);
      ExifEntry *longitude_ref_entry = exif_content_get_entry(ed->ifd[EXIF_IFD_0], EXIF_TAG_GPS_LONGITUDE_REF);
      ExifEntry *longitude_entry = exif_content_get_entry(ed->ifd[EXIF_IFD_0], EXIF_TAG_GPS_LONGITUDE);
      if (latitude_ref_entry && latitude_entry && longitude_ref_entry && longitude_entry) {

	/* todo: find examples of how to extract GPS data */
	
      } else {
	fprintf(stderr, "No gps data in %s", file_old_name);
      }
    }

    for (try_tag_p = tags_to_try;
	 (try_tag = *try_tag_p) != 0;
	 try_tag_p++) {

      ExifEntry *entry = exif_content_get_entry(ed->ifd[EXIF_IFD_0], try_tag);

      if (entry) {

        /* Get the contents of the tag in human-readable form */
        exif_entry_get_value(entry, date_string_buf, sizeof(date_string_buf));

	if (date_string_buf[4] == ':' &&
	    date_string_buf[7] == ':' &&
	    date_string_buf[10] == ' ' &&
	    date_string_buf[13] == ':' &&
	    date_string_buf[16] == ':') {
	  if (target_dir != NULL) {
	    strncpy(file_new_name, target_dir, 1023);
	    remaining = 1023 - strlen(target_dir);
	  } else {
	    char *last_slash = strrchr(file_old_name, '/');
	    if (last_slash) {
	      last_slash++;
	      int upto = last_slash - file_old_name;
	      strncpy(file_new_name, file_old_name, upto);
	      file_new_name[upto] = '\0';
	      remaining -= upto;
	    } else {
	      file_new_name[0] = 0;
	    }
	  }
	  date_string_buf[4] = '-';
	  date_string_buf[7] = '-';
	  date_string_buf[10] = time_separator;
	  date_string_buf[13] = hms;
	  date_string_buf[16] = hms;
	  strncat(file_new_name, date_string_buf, remaining);
	  remaining -= strlen(date_string_buf);
	  strncat(file_new_name, ".jpg", remaining);
	  if (!dry_run) {
	    /* todo: search for file_new_name in deletion_v */
	  }
	} else {
	  printf("Date format for \"%s\" not as expected\n", file_old_name);
	}
	break;
      }
    }

    /* Free the EXIF data */
    exif_data_unref(ed);
  }
  return 0;
}

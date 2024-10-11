#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "statustext.h"
char statustext[512];
char *status_dirpath;

#define STATUS_MAX_FILE  10
#define STATUS_SEP_LEN   sizeof(status_separator) - 1

const char status_separator[] = " / ";

int
render_statustext_filter(const struct dirent *entry)
{
  return entry->d_type == DT_REG;
}

void
render_statustext(void)
{
  time_t now = time(NULL);
  struct tm *tm;
  char dtbuf[100];
  char est[9], utc[9], jst[9], ltime[9], ldate[20];
  char *p = dtbuf;

  setenv("TZ", ":EST", 1);
  tm = localtime(&now);
  strftime(est, sizeof(est), "%R", tm);

  tm = gmtime(&now);
  strftime(utc, sizeof(utc), "%R", tm);

  setenv("TZ", ":Asia/Tokyo", 1);
  tm = localtime(&now);
  strftime(jst, sizeof(jst), "%R", tm);

  setenv("TZ", ":Asia/Bangkok", 1);
  tm = localtime(&now);
  strftime(ldate, sizeof(ldate), "%F (%a)", tm);
  strftime(ltime, sizeof(ltime), "%T", tm);

  p += snprintf(p, sizeof(dtbuf) - (p - dtbuf), "EST: %c%c%s%c%c  ", 1, 0x25, est, 1, 0x20);
  p += snprintf(p, sizeof(dtbuf) - (p - dtbuf), "UTC: %c%c%s%c%c  ", 1, 0x26, utc, 1, 0x20);
  p += snprintf(p, sizeof(dtbuf) - (p - dtbuf), "JST: %c%c%s%c%c  ", 1, 0x27, jst, 1, 0x20);
  p += snprintf(p, sizeof(dtbuf) - (p - dtbuf), "%s  %c%c%s%c%c [%d]",
		ldate, 1, 0x28, ltime, 1, 0x20, tm->tm_year + 1900 + 543);

  const size_t dt_len = p - dtbuf + 1;
  p = statustext;

  if (!status_dirpath) {
    strcpy(p, dtbuf);
    return;
  }

  struct dirent **namelist;
  const int n = scandir(status_dirpath, &namelist, render_statustext_filter, alphasort);
  if (n == -1) {
    p += snprintf(p, sizeof(statustext) - dt_len, "(scandir error) ");
  } else {
    const char *statbuf_cap = statustext + sizeof(statustext) - dt_len;
    for (int i = 0; i < n; i++) {
      if (i >= STATUS_MAX_FILE || p >= statbuf_cap) {
	free(namelist[i]);
	continue;
      }
      char filepath[300];
      snprintf(filepath, sizeof(filepath), "%s/%s", status_dirpath, namelist[i]->d_name);
      free(namelist[i]);

      const size_t remaining = statbuf_cap - p;

      FILE *fp = fopen(filepath, "a+");
      if (fp == NULL) {
	p += snprintf(p, remaining, "(%d: fopen error: [%s]) ", i, filepath);
	continue;
      }

      char readbuf[remaining];
      int r = fread(readbuf, 1, sizeof(readbuf), fp);
      fclose(fp);

      if (r < 1) continue;

      const char *rend = readbuf + r;
      char *rp = readbuf;
      while (rp < rend) {
	switch (*rp) {
	case '\n':
	case '\r':
	  if (statbuf_cap - p < STATUS_SEP_LEN) {
	    rp = readbuf + remaining;
	    break;
	  }

	  memcpy(p, status_separator, STATUS_SEP_LEN);
	  p += STATUS_SEP_LEN;
	  break;
	default:
	  *p = *rp;
	  p++;
	}
	rp++;
      }
    }

    free(namelist);
  }

  strcpy(p, dtbuf);
}

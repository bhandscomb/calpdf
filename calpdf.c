/* calpdf.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <setjmp.h>
#include <time.h>

#include <hpdf.h>

#define A4_width 595
#define A4_height 842

#define INVERT_Y(y) (A4_height-(y))

#define SIZE_YEAR 40

#define SIZE_FOOTER 9

#define SHADE_GREY 0.95

jmp_buf env;

HPDF_Doc pdf;
HPDF_Page page;
HPDF_Font font_normal, font_day;

const char *monthnames[] =
{
  "January", "February", "March",
  "April", "May", "June",
  "July", "August", "September",
  "October", "November", "December"
};

const char *daynames[] =
{
  "S", "M", "T", "W", "T", "F", "S"
};

int days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

const char footerstr[] =
  "Year view by calpdf, based on Solaris/CDE Calendar Manager";
const char footerstr2[] =
  "calpdf by Brian Handscomb";

void
error_handler (HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data)
{
  printf
  (
    "ERROR: error_no=%04X detail_no=%d\n",
    (unsigned int) error_no,
    (int) detail_no
  );
  longjmp (env, 1);
}

void
do_year_header (int year)
{
  HPDF_REAL textwidth;
  char yearstr[10];
  sprintf (yearstr, "%d", year);
  HPDF_Page_GSave (page);
  HPDF_Page_BeginText (page);
  HPDF_Page_SetFontAndSize (page, font_normal, SIZE_YEAR);
  textwidth = HPDF_Page_TextWidth (page, yearstr);
  HPDF_Page_SetTextMatrix
  (
    page, 2.0, 0, 0, 1.0, A4_width / 2.0 - textwidth, INVERT_Y (102)
  );
  HPDF_Page_ShowText (page, yearstr);
  HPDF_Page_EndText (page);
  HPDF_Page_GRestore (page);
}

void
do_month_boxes ()
{
  int x, y;
  HPDF_Page_GSave (page);
  HPDF_Page_SetRGBFill (page, SHADE_GREY, SHADE_GREY, SHADE_GREY);
  for (x = 0; x < 3; x++)
  {
    for (y = 0; y < 4; y++)
    {
      HPDF_Page_Rectangle
      (
        page,
        72 + (x * 153),
        INVERT_Y (154 + (y * 136)),
        145,
        18
      );
      HPDF_Page_Fill (page);
    }
  }
  HPDF_Page_GRestore (page);
}

void
do_month_names ()
{
  int n, x, y;
  HPDF_REAL textwidth;
  HPDF_Page_BeginText (page);
  HPDF_Page_SetFontAndSize (page, font_normal, 16);
  for (n = 0; n < 12; n++)
  {
    x = 72 + ( (n % 3) * 153 );
    y = 150 + ( (n / 3) * 136 );
    textwidth = HPDF_Page_TextWidth (page, monthnames[n]);
    HPDF_Page_TextOut
    (
      page,
      x + (145.0 - textwidth) / 2.0,
      INVERT_Y (y),
      monthnames[n]
    );
  }
  HPDF_Page_EndText (page);
}

void
do_month (int m, int year)
{
  int d, dd, xorg, x, y;
  struct tm *mydate;
  __attribute__((unused)) time_t mytimet;
  char datestr[4];
  HPDF_Page_BeginText (page);
  HPDF_Page_SetFontAndSize (page, font_day, 12);
  xorg = 75 + ( (m % 3) * 153 );
  x = xorg + 4;
  y = 166 + ( (m / 3) * 136 );
  for (d = 0; d < 7; d++)
  {
    HPDF_Page_TextOut (page, x, INVERT_Y (y), daynames[d]);
    x += 21;
  }
  mydate = calloc (1, sizeof (struct tm));
  if (mydate == NULL)
    return;
  mydate->tm_mday = 1;
  mydate->tm_mon = m;
  mydate->tm_year = year - 1900;
  mytimet = mktime (mydate);
  x = xorg + mydate->tm_wday * 21;
  y += 14;
  d = mydate->tm_wday;
  HPDF_Page_SetFontAndSize (page, font_normal, 12);
  for (dd = 1; dd <= days[m]; dd++)
  {
    sprintf (datestr, "%d", dd);
    if (dd < 10)
      x += 6;
    HPDF_Page_TextOut (page, x, INVERT_Y (y), datestr);
    if (dd < 10)
      x += 15;
    else
      x += 21;
    d++;
    if (d >= 7)
    {
      x = xorg;
      y += 14;
      d = 0;
    }
  }
  HPDF_Page_EndText (page);
}

void
do_footer ()
{
  HPDF_REAL textwidth;
  HPDF_Page_GSave (page);
  HPDF_Page_BeginText (page);
  HPDF_Page_SetFontAndSize (page, font_normal, SIZE_FOOTER);
  textwidth = HPDF_Page_TextWidth (page, footerstr);
  HPDF_Page_TextOut
  (
    page,
    523.0 - textwidth,
    INVERT_Y (726),
    footerstr
  );
  textwidth = HPDF_Page_TextWidth (page, footerstr2);
  HPDF_Page_TextOut
  (
    page,
    523.0 - textwidth,
    INVERT_Y (740),
    footerstr2
  );
  HPDF_Page_EndText (page);
  HPDF_Page_GRestore (page);
}

int
main (int argc, char *argv[])
{
  static char pdfname[100];
  int year, m;
  if (argc < 2)
    return 0;
  year = atoi (argv[1]);
  if ( (year < 1900) || (year > 2300) )
  {
    puts ("Year out of range");
    return 0;
  }
  if ((year % 4) == 0)
    days[1]++;
  if ((year % 100) == 0)
    days[1]--;
  if ((year % 400) == 0)
    days[1]++;
  sprintf (pdfname, "%d.pdf", year);
  pdf = HPDF_New (error_handler, NULL);
  if (!pdf)
  { 
    puts ("Unable to create PdfDoc");
    return 1;
  }
  if (setjmp (env))
  {
    HPDF_Free (pdf);
    return 1;
  }
  HPDF_SetCompressionMode (pdf, HPDF_COMP_ALL);
  HPDF_SetInfoAttr (pdf, HPDF_INFO_CREATOR, "calpdf");
  font_normal = HPDF_GetFont (pdf, "Times-Bold", NULL);
  font_day = HPDF_GetFont (pdf, "Courier", NULL);
  page = HPDF_AddPage (pdf);
  HPDF_Page_SetSize (page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);
  do_year_header (year);
  do_month_boxes ();
  do_month_names ();
  for (m = 0; m < 12; m++)
    do_month (m, year);
  do_footer ();
  HPDF_SaveToFile (pdf, pdfname);
  HPDF_Free (pdf);
  return 0;
}


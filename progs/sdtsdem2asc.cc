// first cut using tutorial from GDAL/OGR:
//   http://gdal.org/gdal_tutorial.html

// See README.gdal-data-model

// Opening the File
// ----------------
//
// Before opening a GDAL supported raster datastore it is necessary to
// register drivers. There is a driver for each supported
// format. Normally this is accomplished with the GDALAllRegister()
// function which attempts to register all known drivers, including
// those auto-loaded from .so files using
// GDALDriverManager::AutoLoadDrivers(). If for some applications it
// is necessary to limit the set of drivers it may be helpful to
// review the code from gdalallregister.cpp. Python automatically
// calls GDALAllRegister() when the gdal module is imported.
//
// Once the drivers are registered, the application should call the
// free standing GDALOpen() function to open a dataset, passing the
// name of the dataset and the access desired (GA_ReadOnly or
// GA_Update).

#include <string>
#include <cstdlib> // atoi
#include <cstdio>

#include "SafeFormat.h"     // local library functions
#include "gdal_priv.h"
#include "cpl_conv.h"       // for CPLMalloc()
#include "ogr_spatialref.h"

using namespace std;
using namespace Loki;   // local library functions

// local func decls
void error_exit(const string& msg);
void get_dataset_info();
string get_spaces(const int);
void show_node_and_children(const OGRSpatialReference* sp,
                            const OGR_SRSNode* parent,
                            const char* pname,
                            const int level
                            );

// global vars
OGRSpatialReference* sp(0);
GDALDataset* dataset(0);
bool info(false);
bool debug(false);
int scalex;
int scaley;
int scalez;
double adfGeoTransform[6];
int az(35);
int el(25);
int pixsize(512*3);

int
main(int argc, char** argv)
{
  if (argc < 2) {
    Printf("Usage: %s <SDTS CATD file> [...options...]\n"
           "\n"
           "Without options, prints grid data in XY format to stdout and\n"
           "  pixel data to stderr.\n"
           "\n"
           "Options:\n"
           "\n"
           "  --chop[=X]  Chop cell heights to a base level of X below the minimum\n"
           "                height (default: 1).  Note that X must be >= 1.\n"
           "  --name=X    Use 'X' as the base for output file names.  Outputs:\n"
           "                X.asc\n"
           "                X-reversed.asc\n"
           "                X.dsp\n"
           "                X.g (with X.r inside, az/el: %d/%d)\n"
           "                X.pix (%dx%d)\n"
           "                X-az35-el45.png\n"
           "\n"
           "  --info      Provides information about the input file and exits.\n"
           "  --debug     For developer use: prints debug data to stdout\n"
           )
      (argv[0])
      (az)(el)
      (pixsize)(pixsize)
      ;
    exit(1);
  }

  int chopel(1);
  bool chop(false);
  string ifil;
  string basename;
  for (int i = 1; i < argc; ++i) {
    string arg(argv[i]);
    string val;
    string::size_type idx = arg.find('=');
    if (idx != string::npos) {
      string ab(arg);
      val = arg.substr(idx+1);
      arg.erase(idx);
      // debug
      if (0) {
        Printf("DEBUG: arg before: '%s'\n")(ab);
        Printf("       arg after:  '%s'\n")(arg);
        Printf("       val:        '%s'\n")(val);
      }
    }
    if (arg[0] == '-') {
      if (arg.find("-i") != string::npos) {
        info = true;
      }
      else if (arg.find("-d") != string::npos) {
        debug = true;
      }
      else if (arg.find("-b") != string::npos) {
        basename = val;
      }
      else if (arg.find("-c") != string::npos) {
        chop = true;
        if (!val.empty()) {
          chopel = atoi(val.c_str());
          if (chopel < 1) {
            Printf("FATAL:  Chop elevation '%d' is less than 1.\n")(chopel);
            exit(1);
          }
        }
      }
    }
    else if (ifil.empty()) {
      // get the CATD file
      ifil = arg;
    }
    else {
      Printf("ERROR:  Unknown arg '%s'...exiting.\n")(arg);
      exit(1);
    }
  }

  bool dofils(basename.empty() ? false : true);

  if (ifil.empty()) {
    Printf("ERROR:  No input file was entered...exiting.\n");
    exit(1);
  }

  // debug
  if (0) {
    Printf("DEBUG: early exit.\n");
    exit(1);
  }

  GDALAllRegister();
  dataset = static_cast<GDALDataset*>(GDALOpen(ifil.c_str(), GA_ReadOnly));
  // Note that if GDALOpen() returns NULL it means the open failed,
  // and that an error messages will already have been emitted via
  // CPLError().
  if (!dataset) {
    Printf("ERROR:  Input file '%s' not found...exiting.\n")(ifil);
    exit(1);
  }

  get_dataset_info();


  // Fetching a Raster Band
  // ----------------------
  //
  // At this time access to raster data via GDAL is done one band at a
  // time. Also, there is metadata, blocksizes, color tables, and
  // various other information available on a band by band basis. The
  // following codes fetches a GDALRasterBand object from the dataset
  // (numbered 1 through GetRasterCount()) and displays a little
  // information about it.

  GDALRasterBand* band;
  int             nBlockXSize, nBlockYSize;
  int             bGotMin, bGotMax;
  double          adfMinMax[2];

  int nb(dataset->GetRasterCount());

  if (info) {
    string s(nb > 1 ? "s" : "");
    string isare(nb > 1 ? "are" : "is");

    Printf("There %s %d raster band%s in this data set.\n"
           "Fetching data for band 1:\n"
           )
      (isare)(nb)(s)
      ;
  }

  band = dataset->GetRasterBand(1);
  band->GetBlockSize(&nBlockXSize, &nBlockYSize);

  int success;
  double no_data_value = band->GetNoDataValue(&success);

  int nx = band->GetXSize();
  int ny = band->GetYSize();

  string Stdout, Stderr;

  FILE* fp1(0);
  FILE* fp2(0);
  vector<string> fils;
  if (!basename.empty()) {
    Stdout = basename + ".asc";
    Stderr = basename + ".info";
    fils.push_back(Stdout);
    fils.push_back(Stderr);

    fp1 = fopen(Stdout.c_str(), "w");
    fp2 = fopen(Stderr.c_str(), "w");
  }

  if (info) {
    printf("Block=%dx%d Type=%s, ColorInterp=%s\n",
           nBlockXSize, nBlockYSize,
           GDALGetDataTypeName(band->GetRasterDataType()),
           GDALGetColorInterpretationName(
             band->GetColorInterpretation())
           );
  }
  else {
    if (dofils) {
      fprintf(fp2, "pixels: %d wide X %d high; scale: %d m X %d m X %d m\n",
              nx, ny, scalex, scaley, scalez);
    }
    else {
      fprintf(stderr, "pixels: %d wide X %d high; scale: %d m X %d m X %d m\n",
              nx, ny, scalex, scaley, scalez);
    }
  }

  if (info) {
    if (nx != nBlockXSize) {
      Printf("WARNING: nx = %d but nBlockXSize = %d\n")(nx)(nBlockXSize);
    }
    if (ny != nBlockYSize) {
      Printf("WARNING: ny = %d but nBlockYSize = %d\n")(ny)(nBlockYSize);
    }
  }

  adfMinMax[0] = band->GetMinimum(&bGotMin);
  adfMinMax[1] = band->GetMaximum(&bGotMax);

  if (!(bGotMin && bGotMax))
    GDALComputeRasterMinMax((GDALRasterBandH)band, TRUE, adfMinMax);

  if (info) {
    printf("Min=%.3f, Max=%.3f\n", adfMinMax[0], adfMinMax[1]);

    if (band->GetOverviewCount() > 0)
      printf("Band has %d overviews.\n", band->GetOverviewCount());

    if (band->GetColorTable())
      printf("Band has a color table with %d entries.\n",
             band->GetColorTable()->GetColorEntryCount());

    Printf("\nEarly exit for '--info' option.\n");
    exit(0);
  }

  // Reading Raster Data
  // -------------------
  //
  // There are a few ways to read raster data, but the most common is
  // via the GDALRasterBand::RasterIO() method. This method will
  // automatically take care of data type conversion, up/down sampling
  // and windowing. The following code will read the first scanline of
  // data into a similarly sized buffer, converting it to floating
  // point as part of the operation.

  float* scanline;
  int    nXSize = band->GetXSize();

  scanline = (float*)CPLMalloc(sizeof(float)*nXSize);
  band->RasterIO(GF_Read,     // Either GF_Read to read a region of
                              // data, or GF_Write to write a region
                              // of data.
                 0,           // The pixel offset to the top left
                              // corner of the region of the band to
                              // be accessed. This would be zero to
                              // start from the left side.
                 0,           // The line offset to the top left
                              // corner of the region of the band to
                              // be accessed. This would be zero to
                              // start from the top.
                 nXSize,      // The width of the region of the band
                              // to be accessed in pixels.
                 1,           // The height of the region of the band
                              // to be accessed in lines.
                 scanline,    // The buffer into which the data should
                              // be read, or from which it should be
                              // written. This buffer must contain at
                              // least nBufXSize * nBufYSize words of
                              // type eBufType. It is organized in
                              // left to right, top to bottom pixel
                              // order. Spacing is controlled by the
                              // nPixelSpace, and nLineSpace
                              // parameters.
                 nXSize,      // The width of the buffer image into
                              // which the desired region is to be
                              // read, or from which it is to be
                              // written.
                 1,           // The height of the buffer image into
                              // which the desired region is to be
                              // read, or from which it is to be
                              // written.
                 GDT_Float32, // The type of the pixel values in the
                              // pData data buffer. The pixel values
                              // will automatically be translated
                              // to/from the GDALRasterBand data type
                              // as needed.
                 0,           // The byte offset from the start of one
                              // pixel value in pData to the start of
                              // the next pixel value within a
                              // scanline. If defaulted (0) the size
                              // of the datatype eBufType is used.
                 0            // The byte offset from the start of one
                              // scanline in pData to the start of the
                              // next. If defaulted (0) the size of
                              // the datatype eBufType * nBufXSize is
                              // used.
                 );

  // work all scanlines
  for (int i = 0; i < ny; ++i) {
    // fill the scanline buffer
    band->RasterIO(GF_Read,
                   0,
                   i, // the scanline number
                   nXSize,
                   1,
                   scanline,
                   nXSize,
                   1,
                   GDT_Float32,
                   0,
                   0
                   );
    // read the scanline
    for (int j = 0; j < nx; ++j) {
      int p = static_cast<int>(scanline[j]);

      if (chop) {
        // adjust elevation
        p -= static_cast<int>(floor(adfMinMax[0])) + chopel;

      }
      // debug
      if (debug) {
        if (p < 0)
          continue;
        Printf("pixel[%d,%d] = %d\n")(j)(i)(p);
      }
      else {
        // print a "pixel"
        if (p < 0)
          p = 0;
        if (dofils)
          fprintf(fp1, " %d", p);
        else
          printf(" %d", p);
      }
    }
    if (dofils) {
      fprintf(fp1, "\n");
    }
    else {
      printf("\n");
    }
  }

  CPLFree(scanline);
  GDALClose(dataset);
  if (fp1)
    fclose(fp1);
  if (fp2)
    fclose(fp2);

  // now do the mged trick
  if (dofils) {
    string rfil(basename + "-reversed.asc");
    string dfil(basename + ".dsp");
    fils.push_back(rfil);
    fils.push_back(dfil);

    string cmd;
    SPrintf(cmd, "tac %s > %s")(Stdout)(rfil);
    system(cmd.c_str());

    cmd.clear();
    SPrintf(cmd, "asc2dsp %s %s")(rfil)(dfil);
    system(cmd.c_str());

    string mfil(basename + ".mged");
    fils.push_back(mfil);

    FILE* fp = fopen(mfil.c_str(), "w");
    string solid(basename + ".s");
    string region(basename + ".r");
    FPrintf(fp,
            "units m\n"
            "in %s dsp f %s %d %d 0 ad %d 1\n"
            "r %s u %s\n"
            )
      (solid)(dfil)(nx)(ny)(scalex)
      (region)(solid)
      ;
    fclose(fp);

    // run the mged script
    string gfil(basename + ".g");
    fils.push_back(gfil);
    cmd.clear();
    unlink(gfil.c_str());
    SPrintf(cmd, "mged -c %s < '%s'")(gfil)(mfil);
    system(cmd.c_str());

    // create the images
    string pixfil;
    SPrintf(pixfil, "%s-az%d-el%d.pix")(basename)(az)(el);
    fils.push_back(pixfil);
    string pngfil;
    SPrintf(pngfil, "%s-az%d-el%d.png")(basename)(az)(el);
    fils.push_back(pngfil);

    unlink(pixfil.c_str());
    unlink(pngfil.c_str());

    cmd.clear();
    SPrintf(cmd, "rt -R -o %s -s%d -a%d -e%d %s %s 1>/dev/null 2>/dev/null")
      (pixfil)(pixsize)(az)(el)(gfil)(region)
      ;
    system(cmd.c_str());

    cmd.clear();
    SPrintf(cmd, "pix-png -s%d %s > %s")
      (pixsize)(pixfil)(pngfil)
      ;
    system(cmd.c_str());

  }
  if (dofils && !fils.empty()) {
    unsigned nf = fils.size();
    string s(nf > 1 ? "s" : "");
    Printf("Normal end.  See file%s:\n")(s);
    for (unsigned i = 0; i < nf; ++i) {
      Printf("  %s\n")(fils[i]);
    }
  }

  // The scanline buffer should be freed with CPLFree() when it is
  // no longer used.
  //
  // The RasterIO call takes the following arguments.
  //
  // CPLErr GDALRasterBand::RasterIO(GDALRWFlag eRWFlag,
  //                                 int nXOff, int nYOff, int nXSize, int nYSize,
  //                                 void * pData, int nBufXSize, int nBufYSize,
  //                                 GDALDataType eBufType,
  //                                 int nPixelSpace,
  //                                 int nLineSpace);
  //
  // Note that the same RasterIO() call is used to read, or write
  // based on the setting of eRWFlag (either GF_Read or GF_Write). The
  // nXOff, nYOff, nXSize, nYSize argument describe the window of
  // raster data on disk to read (or write). It doesn't have to fall
  // on tile boundaries though access may be more efficient if it
  // does.
  //
  // The pData is the memory buffer the data is read into, or written
  // from. It's real type must be whatever is passed as eBufType, such
  // as GDT_Float32, or GDT_Byte. The RasterIO() call will take care
  // of converting between the buffer's data type and the data type of
  // the band. Note that when converting floating point data to
  // integer RasterIO() rounds down, and when converting source values
  // outside the legal range of the output the nearest legal value is
  // used. This implies, for instance, that 16bit data read into a
  // GDT_Byte buffer will map all values greater than 255 to 255, the
  // data is not scaled!
  //
  // The nBufXSize and nBufYSize values describe the size of the
  // buffer. When loading data at full resolution this would be the
  // same as the window size. However, to load a reduced resolution
  // overview this could be set to smaller than the window on disk. In
  // this case the RasterIO() will utilize overviews to do the IO more
  // efficiently if the overviews are suitable.
  //
  // The nPixelSpace, and nLineSpace are normally zero indicating that
  // default values should be used. However, they can be used to
  // control access to the memory data buffer, allowing reading into a
  // buffer containing other pixel interleaved data for instance.
  //
  // Closing the Dataset
  // -------------------
  //
  // Please keep in mind that GDALRasterBand objects are owned by
  // their dataset, and they should never be destroyed with the C++
  // delete operator. GDALDataset's can be closed by calling
  // GDALClose() (it is NOT recommended to use the delete operator on
  // a GDALDataset for Windows users because of known issues when
  // allocating and freeing memory across module boundaries. See the
  // relevant topic on the FAQ). Calling GDALClose will result in
  // proper cleanup, and flushing of any pending writes. Forgetting to
  // call GDALClose on a dataset opened in update mode in a popular
  // format like GTiff will likely result in being unable to open it
  // afterwards.

} // main

void
get_dataset_info()
{
  // Getting Dataset Information
  // ---------------------------
  //
  // As described in the GDAL Data Model, a GDALDataset contains a
  // list of raster bands, all pertaining to the same area, and having
  // the same resolution. It also has metadata, a coordinate system, a
  // georeferencing transform, size of raster and various other
  // information.

  // adfGeoTransform[0] /* top left x */
  // adfGeoTransform[1] /* w-e pixel resolution */
  // adfGeoTransform[2] /* rotation, 0 if image is "north up" */
  // adfGeoTransform[3] /* top left y */
  // adfGeoTransform[4] /* rotation, 0 if image is "north up" */
  // adfGeoTransform[5] /* n-s pixel resolution */

  // If we wanted to print some general information about the dataset
  // we might do the following:

  if (dataset->GetGeoTransform(adfGeoTransform) == CE_None) {
    if (info) {
      printf("Origin = (%.6f,%.6f)\n",
             adfGeoTransform[0], adfGeoTransform[3]);

      printf("Pixel Size = (%.6f,%.6f)\n",
             adfGeoTransform[1], adfGeoTransform[5]);
    }
  }

  scalex = static_cast<int>(floor(adfGeoTransform[1]));
  scaley = static_cast<int>(floor(adfGeoTransform[5]));
  // use negative of scaley since we reverse the output
  scaley *= -1;

  if (!info && (scalex != scaley)) {
    Printf("FATAL: cell scale x (%d) != cell scale y (%d)\n")(scalex)(scaley);
    exit(1);
  }
  scalez = 1;

  // check scalez
  if (dataset->GetProjectionRef()) {
    const char* s = dataset->GetProjectionRef();
    if (!sp)
      sp = new OGRSpatialReference(s);
    const OGR_SRSNode* node = sp->GetAttrNode("UNIT");
    string unit(sp->GetAttrValue("UNIT", 0));
    if (unit != "Meter") {
      Printf("FATAL:  Cell unit is '%s' instead of 'Meter'.\n")(unit);
      exit(1);
    }
    else {
      int val = atoi(sp->GetAttrValue("UNIT", 1));
      if (val != 1) {
        Printf("FATAL:  Cell z scale is '%d' instead of '1'.\n")(val);
        exit(1);
      }
    }
  }

  if (info) {
    char** flist2 = dataset->GetFileList();
    CPLStringList flist(flist2, false);
    int nf = flist.size();
    if (nf) {
      printf("Data set files:\n");
      for (int i = 0; i < nf; ++i) {
        const CPLString& s = flist[i];
        printf("  %s\n", s.c_str());
      }
    }
    CSLDestroy(flist2);

    char** dlist2 = dataset->GetMetadata();
    CPLStringList dlist(dlist2, false);
    int nd = dlist.size();
    if (nd) {
      printf("Dataset Metadata:\n");
      for (int i = 0; i < nd; ++i) {
        const CPLString& s = dlist[i];
        printf("  %s\n", s.c_str());
      }
    }

    GDALDriver* driver = dataset->GetDriver();

    char** mlist2 = driver->GetMetadata();
    CPLStringList mlist(mlist2, false);
    int nm = mlist.size();
    if (nm) {
      printf("Driver Metadata:\n");
      for (int i = 0; i < nm; ++i) {
        const CPLString& s = mlist[i];
        printf("  %s\n", s.c_str());
      }
    }

    printf("Driver: %s/%s\n",
           driver->GetDescription(),
           driver->GetMetadataItem(GDAL_DMD_LONGNAME));

    printf("Size is %dx%dx%d\n",
           dataset->GetRasterXSize(), dataset->GetRasterYSize(),
           dataset->GetRasterCount());

    if (dataset->GetProjectionRef()) {
      const char* s = dataset->GetProjectionRef();
      if (!sp)
        sp = new OGRSpatialReference(s);

      vector<string> nodes;
      nodes.push_back("PROJCS");
      nodes.push_back("GEOGCS");
      nodes.push_back("DATUM");
      nodes.push_back("SPHEROID");
      nodes.push_back("PROJECTION");

      printf("Projection is:\n");

      for (unsigned i = 0; i < nodes.size(); ++i) {
        const char* name = nodes[i].c_str();
        const OGR_SRSNode* node = sp->GetAttrNode(name);
        if (!node) {
          printf("  %s (NULL)\n", name);
          continue;
        }
        int level = 0;
        show_node_and_children(sp, node, name, level); // recursive
      }

      /*
      const char* projcs   = sp.GetAttrNode("PROJCS");

      const char* geogcs   = sp.GetAttrValue("GEOGCS");
      const char* datum    = sp.GetAttrValue("DATUM");
      const char* spheroid = sp.GetAttrValue("SPHEROID");
      const char* project  = sp.GetAttrValue("PROJECTION");

      printf("  %s\n", projcs);
      printf("  %s\n", geogcs);
      printf("  %s\n", datum);
      printf("  %s\n", spheroid);
      printf("  %s\n", project);

      */
      //printf("Projection is '%s'\n", dataset->GetProjectionRef());
    }

  }

} // get_dataset_info

string
get_spaces(const int n)
{
  string s("");
  for (int i = 0; i < n; ++i)
    s += "  ";
  return s;
} // get_spaces

void
show_node_and_children(const OGRSpatialReference* sp,
                       const OGR_SRSNode* parent,
                       const char* pname,
                       const int level
                       )
{
  string spaces = get_spaces(level);
  int nc = parent->GetChildCount();
  printf("  %s%s [%d children]:\n",
         spaces.c_str(),
         pname, nc);
  for (int j = 0; j < nc; ++j) {
    const char* cname = sp->GetAttrValue(pname, j);
    // the child may be a  parent
    const OGR_SRSNode* cnode = sp->GetAttrNode(cname);
    if (cnode) {
      show_node_and_children(sp, cnode, cname, level+1);
    }
    else {
      printf("    %d: '%s'\n", j, cname);
    }
  }

} // show_node_and_children

void
error_exit(const string& msg)
{
  Printf("FATAL: %s\n")(msg);
  exit(1);
} // error_exit

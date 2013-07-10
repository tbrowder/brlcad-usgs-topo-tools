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

#include "SafeFormat.h" // local library functions
#include "gdal_priv.h"
#include "cpl_conv.h"   // for CPLMalloc()

using namespace std;
using namespace Loki;   // local library functions

int
main(int argc, char** argv)
{
  if (argc < 2) {
    Printf("Usage: %s <SDTS CATD file> [...options...]\n"
           "\n"
           "Options:\n"
           "\n"
           "  --info   Provides information about the input file and exits.\n"
           "  --debug  For developer use.\n"
           )
      (argv[0])
      ;
    exit(1);
  }

  string ifil;
  bool info(false);
  bool debug(false);
  for (int i = 1; i < argc; ++i) {
    string arg(argv[i]);
    if (arg[0] == '-') {
      if (arg.find("-i") != string::npos) {
        info = true;
      }
      else if (arg.find("-d") != string::npos) {
        debug = true;
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
  if (ifil.empty()) {
    Printf("ERROR:  No input file was entered...exiting.\n");
    exit(1);
  }

  GDALAllRegister();
  GDALDataset* dataset
    = (GDALDataset*)GDALOpen(ifil.c_str(), GA_ReadOnly);
  if (!dataset) {
    Printf("ERROR:  Input file '%s' not found...exiting.\n")(ifil);
    exit(1);
  }

  // Note that if GDALOpen() returns NULL it means the open failed,
  // and that an error messages will already have been emitted via
  // CPLError(). If you want to control how errors are reported to the
  // user review the CPLError() documentation. Generally speaking all
  // of GDAL uses CPLError() for error reporting. Also, note that
  // pszFilename need not actually be the name of a physical file
  // (though it usually is). It's interpretation is driver dependent,
  // and it might be an URL, a filename with additional parameters
  // added at the end controlling the open or almost anything. Please
  // try not to limit GDAL file selection dialogs to only selecting
  // physical files.
  //
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

  double adfGeoTransform[6];

  printf("Driver: %s/%s\n",
          dataset->GetDriver()->GetDescription(),
          dataset->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME));

  printf("Size is %dx%dx%d\n",
         dataset->GetRasterXSize(), dataset->GetRasterYSize(),
         dataset->GetRasterCount());

  if (dataset->GetProjectionRef())
    printf("Projection is `%s'\n", dataset->GetProjectionRef());

  if (dataset->GetGeoTransform(adfGeoTransform) == CE_None) {
    printf("Origin = (%.6f,%.6f)\n",
           adfGeoTransform[0], adfGeoTransform[3]);

    printf("Pixel Size = (%.6f,%.6f)\n",
           adfGeoTransform[1], adfGeoTransform[5]);
  }

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

  band = dataset->GetRasterBand(1);
  band->GetBlockSize(&nBlockXSize, &nBlockYSize);
  printf("Block=%dx%d Type=%s, ColorInterp=%s\n",
          nBlockXSize, nBlockYSize,
          GDALGetDataTypeName(band->GetRasterDataType()),
          GDALGetColorInterpretationName(
            band->GetColorInterpretation()));

  adfMinMax[0] = band->GetMinimum(&bGotMin);
  adfMinMax[1] = band->GetMaximum(&bGotMax);
  if (!(bGotMin && bGotMax))
    GDALComputeRasterMinMax((GDALRasterBandH)band, TRUE, adfMinMax);

  printf("Min=%.3fd, Max=%.3f\n", adfMinMax[0], adfMinMax[1]);

  if (band->GetOverviewCount() > 0)
    printf("Band has %d overviews.\n", band->GetOverviewCount());

  if (band->GetColorTable())
    printf("Band has a color table with %d entries.\n",
            band->GetColorTable()->GetColorEntryCount());

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

  scanline = (float*) CPLMalloc(sizeof(float)*nXSize);
  band->RasterIO(GF_Read, 0, 0, nXSize, 1,
                 scanline, nXSize, 1, GDT_Float32,
                 0, 0);

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

}

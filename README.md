BRL-CAD USGS Topo Tools
=======================

Tools to convert USGA topological (topo) elevation data in various formats to an ASCII format for converting to a BRL-CAD DSP object.

The BRL-CAD project is located at <http://brlcad.org/>.
 
Current converters are available or are under development for:

* GMTED2010
* SDTS DEM

The tools use the GDAL/OGR library which is available from <http://gdal.org/>.

The build system uses CMake available from <http://gmake.org/>.

USGS elevation data used for this project were obtained from several sources:

* GMTED2010: The USGS Earth Explorer at <http://earthexplorer.usgs.gov/>. Note that registration is required to download data (registration is free).
* SDTS DEM: The Geo Community site at <http://data.geocomm.com/dem/demdownload.html/>. Note that you are required to have a Geo Community account for any downloads (membership is free).
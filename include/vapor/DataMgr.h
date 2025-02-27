#include <vector>
#include <iostream>
#include <list>
#include "vapor/VAssert.h"
#include <vapor/BlkMemMgr.h>
#include <vapor/DC.h>
#include <vapor/MyBase.h>
#include <vapor/RegularGrid.h>
#include <vapor/StretchedGrid.h>
#include <vapor/LayeredGrid.h>
#include <vapor/CurvilinearGrid.h>
#include <vapor/UnstructuredGrid2D.h>
#include <vapor/UDUnitsClass.h>
#include <vapor/GridHelper.h>
#include <vapor/DerivedVarMgr.h>

#ifndef DataMgvV3_0_h
    #define DataMgvV3_0_h

using namespace std;

namespace VAPoR {
class PipeLine;
class DerivedVar;
class DerivedDataVar;
class DerivedCoordVar;

//! \class DataMgr
//! \brief A cache based data reader
//! \author John Clyne
//!
//! The DataMgr class is an abstract class that defines public methods for
//! accessing (reading) 1D, 2D and 3D field variables. The class implements a
//! memory cache to speed data access -- once a variable is read it is
//! stored in cache for subsequent access. The DataMgr class is abstract:
//! it declares a number of public and protected pure virtual methods
//! that must be
//! implemented by specializations of this class to allow access to particular
//! file formats.
//!
//! This class inherits from Wasp::MyBase. Unless otherwise documented
//! any method that returns an integer value is returning status. A negative
//! value indicates failure. Error messages are logged via
//! Wasp::MyBase::SetErrMsg().
//!
//! Methods that return a boolean do
//! not, unless otherwise documented, log an error message upon
//! failure (return of false).
//!
//! \param level
//! \parblock
//! Grid refinement level for multiresolution variables.
//! Compressed variables in the VDC have a multi-resolution
//! representation: the sampling grid for multi-resolution variables
//! is hierarchical, and the dimension lengths of adjacent levels in the
//! hierarchy differ by a factor of two. The \p level parameter is
//! used to select a particular depth of the hierarchy.
//!
//! To provide maximum flexibility as well as compatibility with previous
//! versions of the VDC the interpretation of \p level is somewhat
//! complex. Both positive and negative values may be used to specify
//! the refinement level and have different interpretations.
//!
//! For positive
//! values of \p level, a value of \b 0 indicates the coarsest
//! member of the
//! grid hierarchy. A value of \b 1 indicates the next grid refinement
//! after the coarsest, and so on. Using postive values the finest level
//! in the hierarchy is given by GetNumRefLevels() - 1. Values of \p level
//! that are greater than GetNumRefLevels() - 1 are treated as if they
//! were equal to GetNumRefLevels() - 1.
//!
//! For negative values of \p level a value of -1 indicates the
//! variable's native grid resolution (the finest resolution available).
//! A value of -2 indicates the next coarsest member in the hierarchy after
//! the finest, and so
//! on. Using negative values the coarsest available level in the hierarchy is
//! given by negating the value returned by GetNumRefLevels(). Values of
//! \p level that are less than the negation of GetNumRefLevels() are
//! treated as if they were equal to the negation of the GetNumRefLevels()
//! return value.
//! \endparblock
//!
//! \param lod
//! \parblock
//! The level-of-detail parameter, \p lod, selects
//! the approximation level for a compressed variable.
//! The \p lod parameter is similar to the \p level parameter in that it
//! provides control over accuracy of a compressed variable. However, instead
//! of selecting the grid resolution the \p lod parameter controls
//! the compression factor by indexing into the \p cratios vector (see below).
//! As with the \p level parameter, both positive and negative values may be
//! used to index into \p cratios and
//! different interpretations.
//!
//! For positive
//! values of \p lod, a value of \b 0 indicates the
//! the first element of \p cratios, a value of \b 1 indicates
//! the second element, and so on up to the size of the
//! \p cratios vector (See DC::GetCRatios()).
//!
//! For negative values of \p lod a value of \b -1 indexes the
//! last element of \p cratios, a value of \b -2 indexes the
//! second to last element, and so on.
//! Using negative values the first element of \p cratios - the greatest
//! compression rate - is indexed by negating the size of the
//! \p cratios vector.
//! \endparblock
//
class VDF_API DataMgr : public Wasp::MyBase {
public:
    //! Constructor for the DataMgr class.
    //!
    //! The DataMgr will attempt to cache previously read data and coordinate
    //! variables in memory. The \p mem_size specifies the requested cache
    //! size in MEGABYTES!!!
    //!
    //! \param[in] format A string indicating the format of data collection.
    //!
    //! \param[in] mem_size Size of memory cache to be created, specified
    //! in MEGABYTES!! If 0, not restriction is placed on the cache size; the DataMgr will
    //! attempt to allocate as much memory is needed.
    //!
    //! \param[in] numthreads Number of parallel execution threads
    //! to be run during encoding and decoding of compressed data. A value
    //! of 0, the default, indicates that the thread count should be
    //! determined by the environment in a platform-specific manner, for
    //! example using sysconf(_SC_NPROCESSORS_ONLN) under *nix OSes.
    //!
    //
    DataMgr(string format, size_t mem_size, int nthreads = 0);

    virtual ~DataMgr();

    //! Initialize the class
    //!
    //! This method must be called to initialize the DataMgr class with
    //! a list of input data files.
    //!
    //! \param[in] files A list of file paths
    //!
    //! \retval status A negative int is returned on failure and an error
    //! message will be logged with MyBase::SetErrMsg()
    //!
    //
    virtual int Initialize(const vector<string> &paths, const std::vector<string> &options);

    //! \copydoc DC::GetDimensionNames()
    //
    std::vector<string> GetDimensionNames() const
    {
        VAssert(_dc);
        return (_dc->GetDimensionNames());
    }

    //! \copydoc DC::GetAtt(string, string, vector<double>&)
    //
    bool GetAtt (string varname, string attname, vector< double > &values) const
    {
        VAssert(_dc);
        return (_dc->GetAtt(varname, attname, values));
    }

    //! \copydoc DC::GetAtt(string, string, vector<long>&)
    //
    bool GetAtt (string varname, string attname, vector<long> &values) const
    {
        VAssert(_dc);
        return (_dc->GetAtt(varname, attname, values));
    }

    //! \copydoc DC::GetAtt(string, string, string&)
    //
    bool GetAtt (string varname, string attname, string &values) const
    {
        VAssert(_dc);
        return (_dc->GetAtt(varname, attname, values));
    }

    //! \copydoc DC::GetAttNames(string)
    //
    std::vector<string> GetAttNames(string varname) const
    {
        VAssert(_dc);
        return (_dc->GetAttNames(varname));
    }

    //! \copydoc DC::GetAttType(string, string)
    //
    DC::XType GetAttType(string varname, string attname) const
    {
        VAssert(_dc);
        return (_dc->GetAttType(varname, attname));
    }

    //! \copydoc DC::GetDimension()
    //
    bool GetDimension(string dimname, DC::Dimension &dimension, long ts) const
    {
        VAssert(_dc);
        return (_dc->GetDimension(dimname, dimension, ts));
    }
    
    //! Returns the length of a dimension at a given timestep
    //!
    //! \retval length A negative int is returned on failure
    //!
    long GetDimensionLength(string name, long ts) const
    {
        VAssert(_dc);
        DC::Dimension dimension;
        bool ok = GetDimension(name, dimension, ts);
        if (ok)
            return dimension.GetLength();
        return -1;
    }

    //! \copydoc DC::GetMeshNames()
    //
    std::vector<string> GetMeshNames() const
    {
        VAssert(_dc);
        return (_dc->GetMeshNames());
    }

    //! \copydoc DC::GetMesh()
    //
    bool GetMesh(string meshname, DC::Mesh &mesh) const;

    DC::Mesh GetMesh(string meshname) const {
        DC::Mesh mesh;
        GetMesh(meshname, mesh);
        return mesh;
    }

    //! Return a list of names for all of the defined data variables.
    //!
    //! This method returns a list of all data variables defined
    //! in the data set.
    //!
    //! \retval list A vector containing a list of all the data variable names
    //!
    //! \sa GetCoordVarNames()
    //!
    //! \test New in 3.0
    virtual std::vector<string> GetDataVarNames() const;

    //! Return a list of data variables with a given spatial dimension rank
    //!
    //! This method returns a list of all data variables defined
    //! in the data set with the specified dimension rank (number of dimensions).
    //! Data variables may have 0 to 3 spatial dimensions
    //!
    //! \param[in] ndim Variable rank (number of dimensions)
    //!
    //! \retval list A vector containing a list of all the data variable names
    //! with the specified number of dimensions (rank).
    //!
    //! \test New in 3.0
    enum class VarType { Any, Scalar, Particle };
    virtual std::vector<string> GetDataVarNames(int ndim, VarType type = VarType::Any) const;

    //! Return a list of names for all of the defined coordinate variables.
    //!
    //! This method returns a list of all coordinate variables defined
    //! in the data set.
    //!
    //! \retval list A vector containing a list of all the coordinate
    //! variable names
    //!
    //! \sa GetDataVarNames()
    //!
    virtual std::vector<string> GetCoordVarNames() const;

    //! Get time coordinates
    //!
    //! Get a sorted list of all time coordinates defined for the
    //! data set. Multiple time coordinate variables may be defined. This method
    //! collects the time coordinates from all time coordinate variables
    //! and sorts them into a single, global time coordinate variable.
    //!
    //! \note Need to deal with different units (e.g. seconds and days).
    //! \note Should methods that take a time step argument \p ts expect
    //! "local" or "global" time steps
    //!
    void GetTimeCoordinates(std::vector<double> &timecoords) const { timecoords = _timeCoordinates; };

    const std::vector<double> &GetTimeCoordinates() const { return (_timeCoordinates); };

    //! Get time coordinate var name
    //!
    //! Return the name of the time coordinate variable. If no time coordinate
    //! variable is defined the empty string is returned.
    //
    string GetTimeCoordVarName() const;

    //! Return an ordered list of a data variable's coordinate names
    //!
    //! Returns a list of a coordinate variable names for the variable
    //! \p varname, ordered from fastest
    //! to slowest. If \p spatial is true and the variable is time varying
    //! the time coordinate variable name will be included. The time
    //! coordinate variable is always
    //! the slowest varying coordinate axis
    //!
    //! \param[in] varname A valid variable name
    //! \param[in] spatial If true only return spatial dimensions
    //!
    //! \param[out] coordvars Ordered list of coordinate variable names.
    //!
    //! \retval Returns true upon success, false if the variable is
    //! not defined.
    //!
    //
    virtual bool GetVarCoordVars(string varname, bool spatial, std::vector<string> &coord_vars) const;
    vector<string> GetVarCoordVars(string varname, bool spatial) const;

    //! Return a data variable's definition
    //!
    //! Return a reference to a DC::DataVar object describing
    //! the data variable named by \p varname
    //!
    //! \param[in] varname A string specifying the name of the variable.
    //! \param[out] datavar A DataVar object containing the definition
    //! of the named Data variable.
    //!
    //! \retval bool If true the method was successful. If false the
    //! named variable is invalid (unknown)
    //!
    //! \sa GetCoordVarInfo()
    //!
    bool GetDataVarInfo(string varname, VAPoR::DC::DataVar &datavar) const;

    //! Return metadata about a data or coordinate variable
    //!
    //! If the variable \p varname is defined as either a
    //! data or coordinate variable its metadata will
    //! be returned in \p var.
    //!
    //! \retval bool If true the method was successful. If false the
    //! named variable is invalid (unknown)
    //!
    //! \sa GetDataVarInfo(), GetCoordVarInfo()
    //
    bool GetBaseVarInfo(string varname, VAPoR::DC::BaseVar &var) const;

    //! Return a coordinate variable's definition
    //!
    //! Return a reference to a DC::CoordVar object describing
    //! the coordinate variable named by \p varname
    //!
    //! \param[in] varname A string specifying the name of the coordinate
    //! variable.
    //! \param[out] coordvar A CoordVar object containing the definition
    //! of the named variable.
    //!
    //! \retval bool If true the method was successful. If false the
    //! named variable is invalid (unknown)
    //!
    //! \sa GetDataVarInfo()
    //!
    bool GetCoordVarInfo(string varname, VAPoR::DC::CoordVar &cvar) const;

    //! Return a boolean indicating whether a variable is time varying
    //!
    //! This method returns \b true if the variable named by \p varname
    //! is defined and it has a time axis dimension. If either of these conditions
    //! is not true the method returns false.
    //!
    //! \param[in] varname A string specifying the name of the variable.
    //! \retval bool Returns true if variable \p varname exists and is
    //! time varying.
    //!
    bool IsTimeVarying(string varname) const;

    //! Return a boolean indicating whether a variable is compressed
    //!
    //! This method returns \b true if the variable named by \p varname is defined
    //! and it has a compressed representation. If either of these conditions
    //! is not true the method returns false.
    //!
    //! \param[in] varname A string specifying the name of the variable.
    //! \retval bool Returns true if variable \p varname exists and is
    //! compressed
    //!
    //! \sa DefineCoordVar(), DefineDataVar(), DC::BaseVar::GetCompressed()
    //
    bool IsCompressed(string varname) const;

    //! Return the time dimension length for a variable
    //!
    //! Returns the number of time steps (length of the time dimension)
    //! for which a variable is defined. If \p varname does not have a
    //! time coordinate 1 is returned. If \p varname is not defined
    //! as a variable a negative int is returned.
    //!
    //! \param[in] varname A string specifying the name of the variable.
    //! \retval count The length of the time dimension, or a negative
    //! int if \p varname is undefined.
    //!
    //! \sa IsTimeVarying()
    //
    int GetNumTimeSteps(string varname) const;

    //! Return the maximum time dimension length for this data set
    //!
    //! Returns the number of time steps (length of the time dimension)
    //! for any variable is defined.
    //
    int GetNumTimeSteps() const;

    //! \copydoc DC::GetNumRefLevels()
    //
    size_t GetNumRefLevels(string varname) const;

    //! \copydoc DC::GetCRatios()
    //
    std::vector<size_t> GetCRatios(string varname) const;

    //! Read and return variable data
    //!
    //! Reads all data for the data or coordinate variable named by \p varname
    //! for the time step, refinement level, and leve-of-detail indicated
    //! by \p ts, \p level, and \p lod, respectively.
    //!
    //! \param[in] ts
    //! An integer offset into the time coordinate variable
    //! returned by GetTimeCoordinates() indicting the time step of the
    //! variable to access.
    //!
    //! \param[in] varname The name of the data or coordinate variable to access
    //!
    //! \param[in] level Grid refinement level. See DataMgr
    //!
    //! \param[in] lod The level-of-detail parameter, \p lod, selects
    //! the approximation level. See DataMgr.
    //
    VAPoR::Grid *GetVariable(size_t ts, string varname, int level, int lod, bool lock = false);

    //! Read and return a variable hyperslab
    //!
    //! This method is identical to the GetVariable() method, however,
    //! a subregion is specified in the user coordinate system.
    //! \p min and \p max specify the minimum and maximum extents of an
    //! axis-aligned bounding box containing the region of interest. The
    //! VAPoR::Grid object returned contains the intersection
    //! between the
    //! specified
    //! hyperslab and the variable's spatial domain (which is not necessarily
    //! rectangular or axis-aligned).
    //! If the requested hyperslab lies entirely outside of the domain of the
    //! requested variable NULL is returned
    //!
    //! \param[in] min A one, two, or three element array specifying the
    //! minimum extents, in user coordinates, of an axis-aligned box defining
    //! the region-of-interest. The spatial dimensionality of the variable
    //! determines the number of elements in \p min.
    //!
    //! \param[in] max A one, two, or three element array specifying the
    //! maximum extents, in user coordinates, of an axis-aligned box defining
    //! the region-of-interest. The spatial dimensionality of the variable
    //! determines the number of elements in \p max.
    //!
    //! \note The Grid structure returned is allocated from the heap.
    //! it is the caller's responsiblity to delete the returned object
    //! when it is no longer in use.
    //!
    VAPoR::Grid *GetVariable(size_t ts, string varname, int level, int lod, CoordType min, CoordType max, bool lock = false);

    VAPoR::Grid *GetVariable(size_t ts, string varname, int level, int lod, DimsType min, DimsType max, bool lock = false);

    //! Compute the coordinate extents of a variable
    //!
    //! This method finds the spatial domain extents of a variable
    //!
    //! This method returns the min and max extents
    //! specified in user
    //! coordinates, of the smallest axis-aligned bounding box that is
    //! guaranteed to contain
    //! the variable(s) indicated by \p varname, and the given refinement level,
    //! \p level
    //!
    int GetVariableExtents(size_t ts, string varname, int level, int lod, CoordType &min, CoordType &max);

    //! Compute the min and max value of a variable
    //!
    //! This method finds the minimum and maximum value of a variable
    //!
    //! \param[out] range A two element vector containing the minimum and maximum
    //! value, respectively, for the variable \p varname at the specified
    //! time step, lod, etc.
    //
    int GetDataRange(size_t ts, string varname, int level, int lod, std::vector<double> &range);

    //! Compute min and max value of a variable within a specified ROI
    //!
    //! This method finds the minimum and maximum value of a variable within
    //! the region of interest (ROI) specified by \p min and \p max. Note, the
    //! results returned by this method are equivalent to calling the
    //! Grid::GetRange() method on a grid returned by DataMgr::GetVariable
    //! using the same arguments provided here.
    //
    int GetDataRange(size_t ts, string varname, int level, int lod, CoordType min, CoordType max, std::vector<double> &range);

    //! \copydoc DC::GetDimLensAtLevel()
    //!
    virtual int GetDimLensAtLevel(string varname, int level, std::vector<size_t> &dims_at_level, long ts) const
    {
        std::vector<size_t> dummy;
        return (GetDimLensAtLevel(varname, level, dims_at_level, dummy, ts));
    }

    //! Return a variable's array dimension lengths
    //!
    //! This method is equivalent to calling GetDimLensAtLevel() with \p level
    //! equal to -1
    //!
    virtual int GetDimLens(string varname, std::vector<size_t> &dims, long ts) { return (GetDimLensAtLevel(varname, -1, dims, ts)); }

    std::vector<size_t> GetDimLens(string varname) {
        std::vector<size_t> dims;
        GetDimLens(varname, dims, 0);
        return dims;
    }

    //! Unlock a floating-point region of memory
    //!
    //! Decrement the lock counter associatd with a
    //! region of memory, and if zero,
    //! unlock region of memory previously locked with GetVariable().
    //! When the lock counter reaches zero the region is simply
    //! marked available for
    //! internal garbage collection during subsequent GetVariable() calls
    //!
    //! \param[in] rg A pointer to a Grid previosly
    //! returned by GetVariable()
    //!
    //! \retval status Returns a non-negative value on success
    //!
    //! \sa GetVariable()
    //
    void UnlockGrid(const VAPoR::Grid *rg);

    //! \copydoc DC::GetNumDimensions(
    //!   string varname
    //! ) const;
    //
    size_t GetNumDimensions(string varname) const;

    //! \copydoc DC:GetVarTopologyDim()
    //!
    size_t GetVarTopologyDim(string varname) const;

    //! \copydoc DC:GetVarGeometryDim()
    //!
    size_t GetVarGeometryDim(string varname) const;

    //! Clear the memory cache
    //!
    //! This method clears the internal memory cache of all entries
    //
    void Clear();

    //! Returns true if indicated data volume is available
    //!
    //! Returns true if the variable identified by the timestep, variable
    //! name, refinement level, and level-of-detail is present in
    //! the data set. Returns false if
    //! the variable is not available.
    //!
    //! \param[in] ts A valid time step between 0 and GetNumTimesteps()-1
    //! \param[in] varname A valid variable name
    //! \param[in] level Refinement level requested.
    //! \param[in] lod Compression level of detail requested.
    //! refinement level contained in the DC.
    //
    virtual bool VariableExists(size_t ts, string varname, int level = 0, int lod = 0) const;

    //! \copydoc DC::GetMapProjection() const;
    //
    virtual string GetMapProjection() const { return (_proj4String); }

    //! \copydoc DC::GetMapProjectionDefault() const;
    //
    virtual string GetMapProjectionDefault() const { return (_proj4StringDefault); }

    #ifdef VAPOR3_0_0_ALPHA

    //!
    //! Add a pipeline stage to produce derived variables
    //!
    //! Add a new pipline stage for derived variable calculation. If a
    //! pipeline already exists with the same
    //! name it is replaced. The output variable names are added to
    //! the list of variables available for this data
    //! set (see GetVariables3D, etc.).
    //!
    //! An error occurs if:
    //!
    //! \li The output variable names match any of the native variable
    //! names - variable names returned via _GetVariables3D(), etc.
    //! \li The output variable names match the output variable names
    //! of pipeline stage previously added with NewPipeline()
    //! \li A circular dependency is introduced by adding \p pipeline
    //!
    //! \retval status A negative int is returned on failure.
    //!
    int NewPipeline(PipeLine *pipeline);

    //!
    //! Remove the named pipline if it exists. Otherwise this method is a
    //! no-op
    //!
    //! \param[in] name The name of the pipeline as returned by
    //! PipeLine::GetName()
    //!
    void RemovePipeline(string name);
    #endif

    //! Return true if the named variable is the output of a pipeline
    //!
    //! This method returns true if \p varname matches a variable name
    //! in the output list (PipeLine::GetOutputs()) of any pipeline added
    //! with NewPipeline()
    //!
    //! \sa NewPipeline()
    //
    bool IsVariableDerived(string varname) const;

    //! Return true if the named variable is availble from the derived
    //! classes data access methods.
    //!
    //! A return value of true does not imply that the variable can
    //! be read (\see VariableExists()), only that it is part of the
    //! data set known to the derived class
    //!
    //! \sa NewPipeline()
    //
    bool IsVariableNative(string varname) const;

    int AddDerivedVar(DerivedDataVar *derivedVar);

    void RemoveDerivedVar(string varname);

    //! Purge the cache of a variable
    //!
    //! \param[in] varname is the variable name
    //!
    void PurgeVariable(string varname);

    class BlkExts {
    public:
        BlkExts(){};
        BlkExts(const DimsType &bmin, const DimsType &bmax);

        void Insert(const DimsType &bcoord, const CoordType &min, const CoordType &max);

        bool Intersect(const CoordType &min, const CoordType &max, DimsType &bmin, DimsType &bmax, int nCoords = 3) const;

        friend std::ostream &operator<<(std::ostream &o, const BlkExts &b);

    private:
        DimsType               _bmin = {{0, 0, 0}};
        DimsType               _bmax = {{0, 0, 0}};
        std::vector<CoordType> _mins;
        std::vector<CoordType> _maxs;
    };

protected:
    //
    // Cache for various metadata attributes
    //
    template<typename C> class VarInfoCache {
    public:
        //
        //
        void Set(size_t ts, std::vector<string> varnames, int level, int lod, string key, const std::vector<C> &values);

        void Set(size_t ts, string varname, int level, int lod, string key, const std::vector<C> &values)
        {
            std::vector<string> varnames;
            varnames.push_back(varname);
            Set(ts, varnames, level, lod, key, values);
        }

        bool Get(size_t ts, std::vector<string> varnames, int level, int lod, string key, std::vector<C> &values) const;

        bool Get(size_t ts, string varname, int level, int lod, string key, std::vector<C> &values) const
        {
            std::vector<string> varnames;
            varnames.push_back(varname);
            return Get(ts, varnames, level, lod, key, values);
        }

        void Purge(size_t ts, std::vector<string> varnames, int level, int lod, string key);
        void Purge(size_t ts, string varname, int level, int lod, string key)
        {
            std::vector<string> varnames;
            varnames.push_back(varname);
            Purge(ts, varnames, level, lod, key);
        }
        void Purge(std::vector<string> varnames);

        void Clear() { _cache.clear(); }

        static string _make_hash(string key, size_t ts, std::vector<string> cvars, int level, int lod);

        void _decode_hash(const string &hash, string &key, size_t &ts, vector<string> &varnames, int &level, int &lod);

    private:
        std::map<string, std::vector<C>> _cache;
    };

    mutable std::map<std::pair<VarType, size_t>, std::vector<string>> _dataVarNamesCache;

    string _format;
    int    _nthreads;
    size_t _mem_size;

    DC *              _dc;
    VAPoR::UDUnits    _udunits;
    VAPoR::GridHelper _gridHelper;

    DerivedVarMgr _dvm;
    bool          _doTransformHorizontal;
    bool          _doTransformVertical;
    string        _openVarName;

    std::vector<double> _timeCoordinates;
    string              _proj4String;
    string              _proj4StringDefault;
    DimsType            _bs;

    typedef struct {
        size_t              ts;
        string              varname;
        int                 level;
        int                 lod;
        DimsType            bmin;
        DimsType            bmax;
        int                 lock_counter;
        void *              blks;
    } region_t;

    // a list of all allocated regions
    std::list<region_t> _regionsList;

    VAPoR::BlkMemMgr *_blk_mem_mgr;

    std::vector<PipeLine *> _PipeLines;

    mutable VarInfoCache<size_t> _varInfoCacheSize_T;
    mutable VarInfoCache<double> _varInfoCacheDouble;
    mutable VarInfoCache<void *> _varInfoCacheVoidPtr;

    std::map<string, BlkExts> _blkExtsCache;

    std::map<const Grid *, vector<float *>> _lockedFloatBlks;
    std::map<const Grid *, vector<int *>>   _lockedIntBlks;

    // Get the immediate variable dependencies of a variable
    //
    std::vector<string> _get_var_dependencies_1(string varname) const;

    // Recursively get all of the dependencies of a list of variables.
    // Handles cycles in the dependency graph
    //
    std::vector<string> _get_var_dependencies_all(std::vector<string> varnames, std::vector<string> dependencies) const;

    // Return true if native data has a transformable horizontal coordinate
    //
    bool _hasHorizontalXForm() const;

    // Return true if native mesh has a transformable horizontal coordinate
    //
    bool _hasHorizontalXForm(string meshname) const;

    bool _get_coord_vars(string varname, std::vector<string> &scvars, string &tcvar) const;

    bool _get_coord_vars(string varname, vector<DC::CoordVar> &scvarsinfo, DC::CoordVar &tcvarinfo) const;

    int _initTimeCoord();

    int _get_default_projection(string &projection);

    VAPoR::RegularGrid *_make_grid_regular(const DimsType &dims, const std::vector<float *> &blkvec, DimsType &bs, DimsType &bmin, const DimsType &bmax) const;

    VAPoR::StretchedGrid *_make_grid_stretched(const DimsType &dims, const std::vector<float *> &blkvec, const DimsType &bs, const DimsType &bmin, const DimsType &bmax) const;

    VAPoR::LayeredGrid *_make_grid_layered(const DimsType &dims, const std::vector<float *> &blkvec, const DimsType &bs, const DimsType &bmin, const DimsType &bmax) const;

    VAPoR::CurvilinearGrid *_make_grid_curvilinear(size_t ts, int level, int lod, const vector<DC::CoordVar> &cvarsinfo, const DimsType &dims, const std::vector<float *> &blkvec, const DimsType &bs,
                                                   const DimsType &bmin, const DimsType &bmax);

    void _ugrid_setup(const DC::DataVar &var, DimsType &vertexDims, DimsType &faceDims, DimsType &edgeDims,
                      UnstructuredGrid::Location &location,    // node,face, edge
                      size_t &maxVertexPerFace, size_t &maxFacePerVertex, long &vertexOffset, long &faceOffset, long ts) const;

    UnstructuredGrid2D *_make_grid_unstructured2d(size_t ts, int level, int lod, const DC::DataVar &dvarinfo, const vector<DC::CoordVar> &cvarsinfo, const DimsType, const vector<float *> &blkvec,
                                                  const DimsType &bs, const DimsType &bmin, const DimsType &bmax, const vector<int *> &conn_blkvec, const DimsType &conn_bs, const DimsType &conn_bmin,
                                                  const DimsType &conn_bmax);

    VAPoR::Grid *_make_grid(size_t ts, int level, int lod, const VAPoR::DC::DataVar &var, const DimsType &roi_dims, const DimsType &dims, const std::vector<float *> &blkvec,
                            const std::vector<DimsType> &bsvec, const std::vector<DimsType> &bminvec, const std::vector<DimsType> &bmaxvec, const vector<int *> &conn_blkvec,
                            const vector<DimsType> &conn_bsvec, const vector<DimsType> &conn_bminvec, const vector<DimsType> &conn_bmaxvec);

    string _get_grid_type(string varname) const;

    int _find_bounding_grid(size_t ts, string varname, int level, int lod, CoordType min, CoordType max, DimsType &min_ui, DimsType &max_ui);

    void _setupCoordVecsHelper(string data_varname, const DimsType &data_dimlens, const DimsType &data_bmin, const DimsType &data_bmax, string coord_varname, int order, DimsType &coord_dimlens,
                               DimsType &coord_bmin, DimsType &coord_bmax, bool structured, long ts) const;

    int _setupCoordVecs(size_t ts, string varname, int level, int lod, const DimsType &min, const DimsType &max, vector<string> &varnames, DimsType &roi_dims, vector<DimsType> &dimsvec,
                        vector<DimsType> &bsvec, vector<DimsType> &bminvec, vector<DimsType> &bmaxvec, bool structured) const;

    int _setupConnVecs(size_t ts, string varname, int level, int lod, vector<string> &varnames, vector<DimsType> &dimsvec, vector<DimsType> &bsvec, vector<DimsType> &bminvec,
                       vector<DimsType> &bmaxvec) const;

    VAPoR::Grid *_getVariable(size_t ts, string varname, int level, int lod, bool lock, bool dataless);

    VAPoR::Grid *_getVariable(size_t ts, string varname, int level, int lod, DimsType min, DimsType max, bool lock, bool dataless);

    int _parseOptions(vector<string> &options);

    template<typename T> T *_get_region_from_cache(size_t ts, string varname, int level, int lod, const DimsType &bmin, const DimsType &bmax, bool lock);

    template<typename T>
    int _get_unblocked_region_from_fs(size_t ts, string varname, int level, int lod, const DimsType &grid_dims, const DimsType &grid_bs, const DimsType &grid_min, const DimsType &grid_max, T *blks);

    template<typename T>
    int _get_blocked_region_from_fs(size_t ts, string varname, int level, int lod, const DimsType &file_bs, const DimsType &file_dims, const DimsType &grid_dims, const DimsType &grid_bs,
                                    const DimsType &grid_min, const DimsType &grid_max, T *blks);

    template<typename T>
    T *_get_region_from_fs(size_t ts, string varname, int level, int lod, const DimsType &grid_dims, const DimsType &grid_bs, const DimsType &grid_bmin, const DimsType &grid_bmax, bool lock);

    template<typename T> T *_get_region(size_t ts, string varname, int level, int lod, int nlods, const DimsType &dims, const DimsType &bs, const DimsType &bmin, const DimsType &bmax, bool lock);

    template<typename T>
    int _get_regions(size_t ts, const std::vector<string> &varnames, int level, int lod, bool lock, const std::vector<DimsType> &dimsvec, const std::vector<DimsType> &bsvec,
                     const std::vector<DimsType> &bminvec, const std::vector<DimsType> &bmaxvec, std::vector<T *> &blkvec);

    void _unlock_blocks(const void *blks);

    std::vector<string> _get_native_variables() const;

    void *_alloc_region(size_t ts, string varname, int level, int lod, DimsType bmin, DimsType bmax, DimsType bs, int element_sz, bool lock, bool fill);

    void _free_region(size_t ts, string varname, int level, int lod, DimsType bmin, DimsType bmax, bool forceFlag = false);

    bool _free_lru();
    void _free_var(string varname);

    int _level_correction(string varname, int &level) const;
    int _lod_correction(string varname, int &lod) const;

    vector<string> _getDataVarNamesDerived(int ndim) const;

    bool _hasCoordForAxis(vector<string> coord_vars, int axis) const;

    string _defaultCoordVar(const DC::Mesh &m, int axis) const;

    void _assignHorizontalCoords(vector<string> &coord_vars) const;

    bool _getVarDimensions(string varname, vector<DC::Dimension> &dimensions, long ts) const;

    bool _getDataVarDimensions(string varname, vector<DC::Dimension> &dimensions, long ts) const;

    bool _getCoordVarDimensions(string varname, vector<DC::Dimension> &dimensions, long ts) const;

    bool _getVarDimNames(string varname, vector<string> &dimnames) const;

    bool _isDataVar(string varname) const
    {
        vector<string> names = GetDataVarNames();
        return (find(names.begin(), names.end(), varname) != names.end());
    }

    bool _isCoordVar(string varname) const
    {
        vector<string> names = GetCoordVarNames();
        return (find(names.begin(), names.end(), varname) != names.end());
    }

    bool _getVarConnVars(string varname, string &face_node_var, string &node_face_var, string &face_edge_var, string &face_face_var, string &edge_node_var, string &edge_face_var) const;

    DerivedVar *     _getDerivedVar(string varname) const;
    DerivedDataVar * _getDerivedDataVar(string varname) const;
    DerivedCoordVar *_getDerivedCoordVar(string varname) const;

    int _openVariableRead(size_t ts, string varname, int level, int lod);

    template<class T> int _readRegion(int fd, const DimsType &min, const DimsType &max, size_t ndims, T *region);
    int                   _closeVariable(int fd);

    template<class T> int _getVar(string varname, int level, int lod, T *data);

    template<class T> int _getVar(size_t ts, string varname, int level, int lod, T *data);

    void _getLonExtents(std::vector<float> &lons, DimsType dims, float &min, float &max) const;

    void _getLatExtents(std::vector<float> &lons, DimsType dims, float &min, float &max) const;

    int _getCoordPairExtents(string lon, string lat, float &lonmin, float &lonmax, float &latmin, float &latmax, long ts);

    int _initProj4StringDefault();

    int _initHorizontalCoordVars();

    int _initVerticalCoordVars();

    bool _hasVerticalXForm() const;

    bool _hasVerticalXForm(string meshname, string &standard_name, string &formula_terms) const;

    bool _hasVerticalXForm(string meshname) const
    {
        string standard_name, formula_terms;
        return (_hasVerticalXForm(meshname, standard_name, formula_terms));
    }

    bool _isCoordVarInUse(string varName) const;

    // Hide public DC::GetDimLensAtLevel by making it private
    //
    virtual int GetDimLensAtLevel(string varname, int level, std::vector<size_t> &dims_at_level, std::vector<size_t> &bs_at_level, long ts) const;
};

};    // namespace VAPoR
#endif

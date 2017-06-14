//
// $Id$
//

#ifndef _Compressor_h_
#define _Compressor_h_

#include <vector>
#include "SignificanceMap.h"
#include "MatWaveWavedec.h"

namespace VAPoR {

//! \class Compressor
//! \brief A class for managing data set metadata
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//!
//! This class performs either lossy compression
//! decomposition on an array with an arbitrary
//! number of dimensions (up to 3 presently). Compression
//! is performed by transforming
//! data with a wavelet transform, sorting the resulting
//! coefficients, and returning the n largest magnitude coefficients.
//
class WASP_API Compressor : public MatWaveWavedec {
public:
    //! Constructor for compressor class
    //!
    //! This construct initializes a Compressor class object for use
    //! in compression or decomposition. The parameter \p wname specifies
    //! the name of the wavelet to use for subsequent wavelet transforms.
    //! The parameter \p wmode specifies the boundary extention mode
    //! to employ for transforms.
    //! For non-periodic data the biorthogonal wavelets with a
    //! with an appropriate symetric extension are recommended.
    //!
    //! \param[in] dims Vector specifying dimensions of arrays to be compressed
    //! \param[in] wname Name of wavelet to use in transform
    //! \param[in] wmode Boundary extention mode
    //!
    //! \sa MatWaveBase, WaveFiltBase
    //
    Compressor(std::vector<size_t> dims, const string &wname, const string &mode);
    Compressor(std::vector<size_t> dims, const string &wname);

    virtual ~Compressor();

    //! Compress an array
    //!
    //! This method compresses an input array and returns a compressed version
    //! of the data. Compression is performed by wavelet transforming the data
    //! and sorting the resulting wavelet coefficients. Only the largest
    //! \p dst_arr_len coefficients are returned.
    //!
    //! \param[in] src_arr The input array. The dimensions are determined
    //! by the constructor's \p dims parameter.
    //! \param[out] dst_arr The output array that will contain the largest
    //! \p dst_arr_len coefficients from the transformed input array. \p dst_arr
    //! must point to enough space to contain \p dst_arr_len elements.
    //! \param[out] dst_arr_len Length of \p dst_arr.
    //! \param[in,out] A signficance map that, upon return, will provide
    //! the coordinates of the output coefficents returned.
    //!
    //! \retval status A negative value indicates failure
    //! \sa SignificanceMap, KeepAppOnOff()
    //
    int Compress(const float *src_arr, float *dst_arr, size_t dst_arr_len, SignificanceMap *sigmap);
    int Compress(const double *src_arr, double *dst_arr, size_t dst_arr_len, SignificanceMap *sigmap);
    int Compress(const int *src_arr, int *dst_arr, size_t dst_arr_len, SignificanceMap *sigmap);
    int Compress(const long *src_arr, long *dst_arr, size_t dst_arr_len, SignificanceMap *sigmap);

    //! Decompress an array previously compressed with Compress()
    //!
    //! \param[in] src_arr The input array containing the coefficients
    //! previsously computed by Compress().
    //! \param[out] dst_arr The output array that will contain uncompressed
    //! array. \p dst_arr
    //! must point to enough space to contain The entire uncompressed array
    //! \param[in] The signficance map that returned by Compress()
    //! for \p src_arr
    //!
    //! \retval status A negative value indicates failure
    //! \sa Compress()
    //
    int Decompress(const float *src_arr, float *dst_arr, SignificanceMap *sigmap);
    int Decompress(const double *src_arr, double *dst_arr, SignificanceMap *sigmap);
    int Decompress(const int *src_arr, int *dst_arr, SignificanceMap *sigmap);
    int Decompress(const long *src_arr, long *dst_arr, SignificanceMap *sigmap);

    //! Decompose an array into a series of approximations of increasingly
    //! better fidelity.
    //!
    //! This method performs a wavelet decomposition of an array, sorts
    //! the resulting coefficients from largest to smallest, and returns
    //! the sorted coefficients as a collection of \p n sets, S<sub>i</sub>.
    //! The absolute value
    //! of each element of a set S<sub>i</sub> will be less than each
    //! element of a set S<sub>i+1</sub>.
    //!
    //! \param[in] src_arr The input array. The dimensions are determined
    //! by the constructor's \p dims parameter.
    //!
    //! \param[out] dst_arr The output array that will contain all of
    //! the sorted coefficients for each collection S<sub>i</sub>.
    //!
    //! \param[in] dst_arr_lens A vector whose size determines the number
    //! of collections S<sub>i</sub>, and whose elements specify the number
    //! of elements in each collection S<sub>i</sub>. The sum of all elements
    //! of \p dst_arr_lens must be less than or equal the total number of wavelet
    //! coefficients generated by the wavelet transform. The total is given
    //! by the size of the vector returned by GetSigMapShape().
    //!
    //! \param[out] sigmaps An array of significance maps, one map for
    //! each coefficient collection, S<sub>i</sub>
    //!
    //! \retval status A negative value indicates failure
    //! \sa SignificanceMap, KeepAppOnOff(), Compress()
    //
    int Decompose(const float *src_arr, float *dst_arr, const vector<size_t> &dst_arr_lens, vector<SignificanceMap> &sigmaps);
    int Decompose(const double *src_arr, double *dst_arr, const vector<size_t> &dst_arr_lens, vector<SignificanceMap> &sigmaps);
    int Decompose(const int *src_arr, int *dst_arr, const vector<size_t> &dst_arr_lens, vector<SignificanceMap> &sigmaps);
    int Decompose(const long *src_arr, long *dst_arr, const vector<size_t> &dst_arr_lens, vector<SignificanceMap> &sigmaps);

    //! Reconstruct a signal decomposed with Decompose()
    //!
    //! This method reconstructs a signal previosly decomposed with Decompose().
    //! Partial reconstructions are possible using one of two methods, both
    //! of which may be combined. In the first the a subset of
    //! coefficient collections S<sub>i</sub> may be used for reconstruction.
    //! In this case missing coefficients are simply treated as having
    //! value zero.  Note that arbitrary S<sub>i</sub> cannot be specified: the
    //! collections must be ordered with <em>i</em> less than or equal to
    //! the total number of collections returned by Decompose().
    //!
    //! The second method of partial reconstruction is to limit the number of
    //! inverse wavelet transforms applied to the coefficients. In this case
    //! the reconstructed array will be coarsened (contain fewer elements)
    //! than the original. The dimensions of the reconstructed array
    //! are given by GetDimension().
    //!
    //! \param[in] src_arr The input array containing the coefficients
    //! previsously computed by Decompose().
    //!
    //! \param[out] dst_arr The output array containing reconstructed signal.
    //! The dimensions of \p dst_arr are determined by GetDimension(). If
    //! \p l is -1, the dimensions will be the same as those specified by
    //! the class constructor's \p dims parameter
    //!
    //! \param[in] sigmaps An array of significance maps previosly returned
    //! by Decompose()
    //!
    //! \param[in] l The refinement level. \p l must be in the range -1 to
    //! max, where max is the value returned by GetNumLevels(). If \p is -1
    //! its value will be set to max. A value of zero coresponds to the
    //! approximation level coefficients.
    //!
    //! \retval status A negative value indicates failure
    //! \sa SignificanceMap, KeepAppOnOff(), Compress()
    //!
    int Reconstruct(const float *src_arr, float *dst_arr, vector<SignificanceMap> &sigmaps, int l);
    int Reconstruct(const double *src_arr, double *dst_arr, vector<SignificanceMap> &sigmaps, int l);
    int Reconstruct(const int *src_arr, int *dst_arr, vector<SignificanceMap> &sigmaps, int l);
    int Reconstruct(const long *src_arr, long *dst_arr, vector<SignificanceMap> &sigmaps, int l);

    //! Return true if the given grid array is compressible
    //!
    //! Return true if the given grid array is compressible based on
    //! it's dimension and the requested wavelet. I.e. returns true
    //! if at least one dimension is wide enough for a single wavelet transform
    //!
    //! \param[in] dims Vector specifying dimensions of arrays to be compressed
    //! \param[in] wname Name of wavelet to use in transform
    //! \param[in] wmode Boundary extention mode
    //!
    //! \sa MatWaveBase, WaveFiltBase
    //!
    // static bool IsCompressible(
    //	std::vector <size_t> dims, const string &wavename, const string &mode
    //);

    //! Returns the shape of significance maps based on constructor
    //!
    //! This method returns the shape of any significance maps configured
    //! by the Compress() or Decompose() method based on parameters passed
    //! to the class constructor.
    //!
    //! \param[out] dims A vector describing the shap (dimensions) of a
    //! significance map
    //!
    //! \sa SignificanceMap, Compress(), Decompose()
    //!
    void GetSigMapShape(std::vector<size_t> &dims) const
    {
        dims.clear();
        dims.push_back(_CLen);
    };

    //! Returns the number of wavelet coefficients resulting from
    //! a forward wavelet transformation of an array.
    //!
    //! Returns the number of wavelet coefficients resulting from
    //! a forward wavelet transformation of an array based on
    //! the parameters supplied to the class constructor. The
    //! number of coefficients resulting from a wavelet transform
    //! is a function of the dimensions of the array, the wavlet
    //! familiy used, the boundary handling method, and the number of
    //! transformation levels.
    //!
    //! \note For periodic boundary handling, or symetric wavelets matched
    //! with appropriate symetric boundary handling, the number of coefficients
    //! in the transform is guaranteed to match the number of coefficients
    //! in the input array.
    //!
    //! \sa Compress(), Decompose()
    //!
    size_t GetNumWaveCoeffs() const { return (_CLen); };

    //! Returns the size of an encoded SignficanceMap()
    //!
    //! Returns the size in bytes of an encoded SignificanceMap()
    //! used to store \p num_entries entries.
    //!
    //! \sa Compress(), Decompose()
    //!
    size_t GetSigMapSize(size_t num_entries) const
    {
        std::vector<size_t> dims;
        dims.push_back(GetNumWaveCoeffs());
        return (SignificanceMap::GetMapSize(dims, num_entries));
    };

    //! Returns the dimensions of a reconstructed array
    //!
    //! This method returns the dimensions of an array reconstructed with
    //! either the Reconstruct() or Decompress() methods. The parameter
    //! \p l specifies the number of inverse transformation passes
    //! applied in the range -1 to max, where max is the value returned by
    //! GetNumLevels(). A value of -1 corresponds to max levels. A value of
    //! zero implies no inverse transformations are performed (the dimensions
    //! returned are those of the wavelet approximation coefficients).
    //!
    //! \param[out] dims The dimensions of the reconstructed array.
    //! \param[in] l The number of inverse transforms to apply
    //!
    //! \sa Compressor(), GetNumLevels()

    void GetDimension(vector<size_t> &dims, int l) const;

    //! Returns the number of transformation levels
    //!
    //! Returns the number of forward or inverse wavelet transformations
    //! applied to compress or fully reconstruct an array. The number of
    //! of transforms is determined by a combination of the wavelet,
    //! the boundary extension handling method, and the dimensions of the
    //! array.
    //!
    //! \retval nlevels Number of transformation levels
    //
    int GetNumLevels() const { return (_nlevels); };

    //! Returns the number of coefficients in the smallest allowable compression
    //!
    //! Returns the minimum number of wavelet coefficients allowable
    //! after compression.
    //! The minimum number of
    //! of coefficients is determined by a combination of the wavelet,
    //! the boundary extension handling method, the dimensions of the
    //! array, and the setting of KeepAppOnOff()
    //!
    //! \retval nlevels Number of transformation levels
    //
    size_t GetMinCompression() const;

    //! Set or get the keep approximations attribute
    //!
    //! When set, this attribute ensures that all wavelet approximation
    //! coefficients are retained during compression or decomposition. Setting
    //! this attribute will decrease the maximum possible compression, but may
    //! significantly improve the fidelity of the approximation
    //!
    bool &KeepAppOnOff() { return (_keepapp); };

    //! Set or get the min range clamping attribute
    //!
    //! When set, this attribute will clamp the minimum data value
    //! reconstructed to the value of ClampMin(). I.e. all data values
    //! reconstructed by Decompress() or Reconstruct() will be greater
    //! than or equal to the value returned by ClampMin(). By default
    //! clamping is disabled.
    //!
    //! \sa ClampMin(), Decompress(), Reconstruct()
    //!
    bool &ClampMinOnOff() { return (_clamp_min_flag); };

    //! Set or get the minimum range clamp value
    //!
    //! \sa ClampMinOnOff(), Decompress(), Reconstruct()
    //!
    double &ClampMin() { return (_clamp_min); };

    //! Set or get the max range clamping attribute
    //!
    //! When set, this attribute will clamp the maximum data value
    //! reconstructed to the value of ClampMax(). I.e. all data values
    //! reconstructed by Decompress() or Reconstruct() will be less
    //! than or equal to the value returned by ClampMax(). By default
    //! clamping is disabled.
    //!
    //! \sa ClampMin(), Decompress(), Reconstruct()
    //!
    bool &ClampMaxOnOff() { return (_clamp_max_flag); };

    //! Set or get the maximum range clamp value
    //!
    //! \sa ClampMaxOnOff(), Decompress(), Reconstruct()
    //!
    double &ClampMax() { return (_clamp_max); };

    //! Set or get the epsilon attribute
    //!
    //! When set, this attribute will compare the absolute value of
    //! reconstructed data with Epsilon(). If abs(v) is less than epsilon
    //! the value of v is set to 0.0.
    //! By default
    //! the epsilon comparison is disabled.
    //!
    //! \sa Epsilon(), Decompress(), Reconstruct()
    //!
    bool &EpsilonOnOff() { return (_epsilon_flag); };

    //! Set or get the epsilon value
    //!
    //! \sa EpsilonOnOff(), Decompress(), Reconstruct()
    //!
    double &Epsilon() { return (_epsilon); };

    static bool CompressionInfo(vector<size_t> dims, const string wavename, bool keepapp, size_t &nlevels, size_t &maxcratio);

    friend std::ostream &operator<<(std::ostream &o, const Compressor &rhs);

private:
    vector<size_t> _dims;        // dimensions of array
    int            _nlevels;     // Number of wavelet transformation levels
    vector<void *> _indexvec;    // used to sort wavelet coefficients
    size_t         _nx;
    size_t         _ny;
    size_t         _nz;
    double *       _C;    // storage for wavelet coefficients
    size_t         _CLen;
    size_t *       _L;    // wavelet coefficient book keeping array
    size_t         _LLen;
    bool           _keepapp;    // if true, approximation coeffs are not used in compression
    bool           _clamp_min_flag;
    bool           _clamp_max_flag;
    bool           _epsilon_flag;
    double         _clamp_min;
    double         _clamp_max;
    double         _epsilon;

    void _Compressor(std::vector<size_t> dims);
};

}    // namespace VAPoR

#endif

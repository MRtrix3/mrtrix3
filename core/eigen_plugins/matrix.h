template <class ImageType> Matrix (const MR::Helper::ConstRow<ImageType>& row) : Base () { operator= (row); }
template <class ImageType> Matrix (const MR::Helper::Row<ImageType>& row) : Base () { operator= (row); }

#define MRTRIX_OP(ARG) \
template <class ImageType> inline Matrix& operator ARG (const MR::Helper::ConstRow<ImageType>& row) { \
  this->resize (row.image.size(row.axis),1); \
  for (row.image.index(row.axis) = 0; row.image.index (row.axis) < row.image.size (row.axis); ++row.image.index (row.axis)) \
    this->operator() (row.image.index (row.axis), 0) ARG row.image.value(); \
  return *this; \
}

MRTRIX_OP(=)

#undef MRTRIX_OP

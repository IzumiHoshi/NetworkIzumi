#include <istream>
#include <algorithm>

template <typename T>
inline T ply_read_ascii(std::istream &is) {
    T data;
    if (is >> data) {
        return data;
    } else {
        is.clear();
        is.ignore();
        return std::numeric_limits<T>::quiet_NaN();
    }
}

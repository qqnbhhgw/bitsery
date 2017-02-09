//
// Created by Mindaugas Vinkelis on 17.1.9.
//

#ifndef TMP_DESERIALIZER_H
#define TMP_DESERIALIZER_H

#include "Common.h"
#include <array>

template<typename Reader>
class Deserializer {
public:
    Deserializer(Reader& r):_reader{r} {};

    template <typename T>
    Deserializer& object(T&& obj) {
        return serialize(*this, std::forward<T>(obj));
    }

    /*
     * value overloads
     */

    template<size_t VSIZE = 0, typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
    Deserializer& value(T& v) {
        static_assert(std::numeric_limits<float>::is_iec559, "");
        static_assert(std::numeric_limits<double>::is_iec559, "");

        constexpr size_t ValueSize = VSIZE == 0 ? sizeof(T) : VSIZE;
        using CT = std::conditional_t<std::is_same<T,float>::value, uint32_t, uint64_t>;
        static_assert(sizeof(CT) == ValueSize, "");
        _reader.template readBytes<ValueSize>(reinterpret_cast<CT&>(v));
        return *this;
    }

    template<size_t VSIZE = 0, typename T, typename std::enable_if<std::is_enum<T>::value>::type* = nullptr>
    Deserializer& value(T& v) {
        constexpr size_t ValueSize = VSIZE == 0 ? sizeof(T) : VSIZE;
        using UT = std::underlying_type_t<T>;
        _reader.template readBytes<ValueSize>(reinterpret_cast<UT&>(v));
        return *this;
    }

    template<size_t VSIZE = 0, typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    Deserializer& value(T& v) {
        constexpr size_t ValueSize = VSIZE == 0 ? sizeof(T) : VSIZE;
        _reader.template readBytes<ValueSize>(v);
        return *this;
    }

    /*
     * text overloads
     */

    template <size_t VSIZE = 1, typename T>
    Deserializer& text(std::basic_string<T>& str) {
        size_t size;
        readLength(size);
		std::vector<T> buf(size);
		_reader.template readBuffer<VSIZE>(buf.data(), size);
		str.assign(buf.data(), size);
//		str.resize(size);
//		if (size)
//			_reader.template readBuffer<VSIZE>(str.data(), size);
        return *this;
    }

    template<size_t VSIZE=1, typename T, size_t N>
    Deserializer& text(T (&str)[N]) {
        size_t size;
        readLength(size);
        _reader.template readBuffer<VSIZE>(str, size);
        str[size] = {};
        return *this;
    }

    /*
     * container overloads
     */

    template <typename T, typename Fnc>
    Deserializer& container(T&& obj, Fnc&& fnc) {
        decltype(obj.size()) size{};
        readLength(size);
        obj.resize(size);
        for (auto& v:obj)
            fnc(v);
        return *this;
    }

    template <size_t VSIZE, typename T>
    Deserializer& container(T& obj) {
        decltype(obj.size()) size{};
        readLength(size);
        obj.resize(size);
        procContainer<VSIZE>(obj);
        return *this;
    }

    template <typename T>
    Deserializer& container(T& obj) {
        decltype(obj.size()) size{};
        readLength(size);
        obj.resize(size);
        using VType = typename T::value_type;
        constexpr auto VSIZE = std::is_arithmetic<VType>::value || std::is_enum<VType>::value ? sizeof(VType) : 0;
        procContainer<VSIZE>(obj);
        return *this;
    }

    /*
     * array overloads (fixed size array (std::array, and c-style array))
     */

    //std::array overloads

    template<typename T, size_t N, typename Fnc>
    Deserializer & array(std::array<T,N> &arr, Fnc && fnc) {
        for (auto& v: arr)
            fnc(v);
        return *this;
    }

    template<size_t VSIZE, typename T, size_t N>
    Deserializer & array(std::array<T,N> &arr) {
        procContainer<VSIZE>(arr);
        return *this;
    }

    template<typename T, size_t N>
    Deserializer & array(std::array<T,N> &arr) {
        constexpr auto VSIZE = std::is_arithmetic<T>::value || std::is_enum<T>::value ? sizeof(T) : 0;
        procContainer<VSIZE>(arr);
        return *this;
    }

    //c-style array overloads

    template<typename T, size_t N, typename Fnc>
    Deserializer& array(T (&arr)[N], Fnc&& fnc) {
        T* tmp = arr;
        for (auto i = 0u; i < N; ++i, ++tmp)
            fnc(*tmp);
        return *this;
    }


private:
    Reader& _reader;
    void readLength(size_t& size) {
        size = {};
		_reader.template readBits<32>(size);
    }
    template <size_t VSIZE, typename T>
    void procContainer(T&& obj) {
        for (auto& v: obj)
            ProcessAnyType<VSIZE>::serialize(*this, v);
    };

};


#endif //TMP_DESERIALIZER_H

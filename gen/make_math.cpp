#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <sstream>
#include <type_traits>

#define VERSION "3.1.15"

#pragma GCC diagnostic ignored "-Wpragmas" // Silence GCC warning about the next line disabling a warning that GCC doesn't have.
#pragma GCC diagnostic ignored "-Wstring-plus-int" // Silence clang warning about `1+R"()"` pattern.

namespace data
{
    const struct {std::string tag, name;} type_list[]
    {
        {"b"     , "bool"              },
        {"c"     , "char"              },
        {"uc"    , "unsigned char"     },
        {"sc"    , "signed char"       },
        {"s"     , "short"             },
        {"us"    , "unsigned short"    },
        {"i"     , "int"               },
        {"u"     , "unsigned int"      },
        {"l"     , "long"              },
        {"ul"    , "unsigned long"     },
        {"ll"    , "long long"         },
        {"ull"   , "unsigned long long"},
        {"f"     , "float"             },
        {"d"     , "double"            },
        {"ld"    , "long double"       },
        {"i8"    , "std::int8_t"       },
        {"u8"    , "std::uint8_t"      },
        {"i16"   , "std::int16_t"      },
        {"u16"   , "std::uint16_t"     },
        {"i32"   , "std::int32_t"      },
        {"u32"   , "std::uint32_t"     },
        {"i64"   , "std::int64_t"      },
        {"u64"   , "std::uint64_t"     },
        {"index_", "std::ptrdiff_t"    },
        {"size_" , "std::size_t"       },
    };
    constexpr int type_list_len = std::extent_v<decltype(type_list)>;

    const std::string fields[4] {"x","y","z","w"};
    constexpr int fields_alt_count = 2;
    const std::string fields_alt[fields_alt_count][4]
    {
        {fields[0], fields[1], fields[2], fields[3]},
        {"r","g","b","a"},
        // "s","t","p","q", // Who uses this anyway.
    };

    const std::string custom_operator_symbol = "/", custom_operator_list[]{"dot","cross"};
}

namespace impl
{
    std::ofstream output_file;

    std::stringstream ss;
    const std::stringstream::fmtflags stdfmt = ss.flags();

    bool at_line_start = 1;
    int indentation = 0;
    int section_depth = 0;

    constexpr const char *indentation_string = "    ", *indentation_string_labels = "  ";

    void init(int argc, char **argv)
    {
        if (argc < 2)
        {
            std::cout << "Expected output file name.";
            std::exit(-1);
        }
        if (argc > 2)
        {
            std::cout << "Invalid usage.";
            std::exit(-1);
        }

        output_file.open(argv[1]);
        if (!output_file)
        {
            std::cout << "Unable to open `" << argv[1] << "`.\n";
            std::exit(-1);
        }
    }
}

template <typename ...P> [[nodiscard]] std::string make_str(const P &... params)
{
    impl::ss.clear();
    impl::ss.str("");
    impl::ss.flags(impl::stdfmt);
    (impl::ss << ... << params);
    return impl::ss.str();
}

void output_str(const std::string &str)
{
    for (const char *ptr = str.c_str(); *ptr; ptr++)
    {
        char ch = *ptr;

        if (ch == '}' && impl::indentation > 0)
            impl::indentation--;

        if (impl::at_line_start)
        {
            if (std::strchr(" \t\r", ch))
                continue;

            for (int i = 0; i < impl::indentation; i++)
                impl::output_file << (i == impl::indentation-1 && ch == '@' ? impl::indentation_string_labels : impl::indentation_string);
            impl::at_line_start = 0;
        }

        if (ch != '@')
            impl::output_file.put(ch == '$' ? ' ' : ch);

        if (ch == '{')
            impl::indentation++;

        if (ch == '\n')
            impl::at_line_start = 1;
    }
}

template <typename ...P> void output(const P &... params)
{
    output_str(make_str(params...));
}

void section(std::string header, std::function<void()> func)
{
    output(header, "\n{\n");
    func();
    output("}\n");
}
void section_sc(std::string header, std::function<void()> func) // 'sc' stands for 'end with semicolon'
{
    output(header, "\n{\n");
    func();
    output("};\n");
}

void decorative_section(std::string name, std::function<void()> func)
{
    output("//{", std::string(impl::section_depth+1, ' '), name, "\n");
    impl::indentation--;
    impl::section_depth++;
    func();
    impl::section_depth--;
    output("//}", std::string(impl::section_depth+1, ' '), name, "\n");
    impl::indentation++;
}

void next_line()
{
    output("\n");
}

int main(int argc, char **argv)
{
    impl::init(argc, argv);

    { // Header
        output(1+R"(
            // mat.h
            // Vector and matrix math
            // Version )", VERSION, R"(
            // Generated, don't touch.

            #pragma once
        )");
        next_line();
    }

    { // Includes
        output(1+R"(
            #include <algorithm>
            #include <cmath>
            #include <cstddef>
            #include <cstdint>
            #include <istream>
            #include <ostream>
            #include <tuple>
            #include <type_traits>
            #include <utility>
        )");
        next_line();
    }

    section("namespace Math", []
    {
        section("inline namespace Vector // Declarations", []
        {
            { // Main templates
                output(1+R"(
                    template <int D, typename T> struct vec;
                    template <int W, int H, typename T> struct mat;
                )");
            }
        });

        section("inline namespace Alias // Short type aliases", []
        {
            { // Type-generic
                // Vectors of specific size
                for (int i = 2; i <= 4; i++)
                    output(" template <typename T> using vec", i, " = vec<", i, ",T>;");
                next_line();

                // Matrices of specific size
                for (int h = 2; h <= 4; h++)
                {
                    for (int w = 2; w <= 4; w++)
                        output(" template <typename T> using mat", w, "x", h, " = mat<", w, ",", h, ",T>;");
                    next_line();
                }

                // Square matrices of specific size
                for (int i = 2; i <= 4; i++)
                    output(" template <typename T> using mat", i, " = mat", i, "x", i, "<T>;");
                next_line();
            }
            next_line();

            { // Size-generic
                for (int i = 0; i < data::type_list_len; i++)
                {
                    const auto &type = data::type_list[i];

                    // Any size
                    output("template <int D> using ", type.tag, "vec = vec<D,", type.name, ">;\n"
                           "template <int W, int H> using ", type.tag, "mat = mat<W,H,", type.name, ">;\n");

                    // Fixed size
                    for (int d = 2; d <= 4; d++)
                        output(" using ", type.tag, "vec", d, " = vec<", d, ',', type.name, ">;");
                    next_line();
                    for (int h = 2; h <= 4; h++)
                    {
                        for (int w = 2; w <= 4; w++)
                            output(" using ", type.tag, "mat", w, "x", h, " = mat<", w, ",", h, ",", type.name, ">;");
                        next_line();
                    }
                    for (int i = 2; i <= 4; i++)
                        output(" using ", type.tag, "mat", i, " = ", type.tag, "mat", i, "x", i, ";");
                    next_line();

                    if (i != data::type_list_len-1)
                        next_line();
                }
            }
        });

        next_line();

        section("inline namespace Utility // Helper templates", []
        {
            output(1+R"(
                // Check if `T` is a vector type (possibly const).
                template <typename T> struct is_vector_impl : std::false_type {};
                template <int D, typename T> struct is_vector_impl<      vec<D,T>> : std::true_type {};
                template <int D, typename T> struct is_vector_impl<const vec<D,T>> : std::true_type {};
                template <typename T> inline constexpr bool is_vector_v = is_vector_impl<T>::value;

                // Checks if none of `P...` are vector types.
                template <typename ...P> inline constexpr bool no_vectors_v = !(is_vector_v<P> || ...);

                // Check if `T` is a matrix type (possibly const).
                template <typename T> struct is_matrix_impl : std::false_type {};
                template <int W, int H, typename T> struct is_matrix_impl<      mat<W,H,T>> : std::true_type {};
                template <int W, int H, typename T> struct is_matrix_impl<const mat<W,H,T>> : std::true_type {};
                template <typename T> inline constexpr bool is_matrix_v = is_matrix_impl<T>::value;

                // Check if `T` is an 'other type' (possbily const), i.e. not a suitable vector/matrix element.
                // Effectively checks for a member `disable_vec_mat_operators` typedef.
                template <typename T, typename = void> struct is_other_impl : std::false_type {};
                template <typename T> struct is_other_impl<T, decltype(std::enable_if<1, typename T::disable_vec_mat_operators>{}, void())> : std::true_type {}; // Note the use of `enable_if` without `_t`. We just need an arbitrary template type here.
                template <typename T> inline constexpr bool is_other_v = is_other_impl<T>::value;

                // Check if a type is a scalar type (i.e. not vector nor matrix nor 'other').
                template <typename T> inline constexpr bool is_scalar_v = !is_vector_v<T> && !is_matrix_v<T> && !is_other_v<T>;

                template <typename A, typename B = void> using enable_if_scalar_t = std::enable_if_t<is_scalar_v<A>, B>;

                // If `T` is a vector (possibly const), returns its element type. Otherwise returns `T`.
                template <typename T> using vec_base_t = typename std::conditional_t<is_vector_v<T>, T, std::enable_if<1,T>>::type;

                // If `T` is a vector (possibly const), returns its size. Otherwise returns 1.
                template <typename T> struct vec_size_impl : std::integral_constant<int, 1> {};
                template <int D, typename T> struct vec_size_impl<      vec<D,T>> : std::integral_constant<int, D> {};
                template <int D, typename T> struct vec_size_impl<const vec<D,T>> : std::integral_constant<int, D> {};
                template <typename T> inline constexpr int vec_size_v = vec_size_impl<T>::value;

                // If `A` is a `[const] vec<D,T>`, returns `[const] vec<D,B>`. Otherwise returns `B`.
                template <typename A, typename B> struct change_vec_base_impl {using type = B;};
                template <int D, typename A, typename B> struct change_vec_base_impl<      vec<D,A>,B> {using type =       vec<D,B>;};
                template <int D, typename A, typename B> struct change_vec_base_impl<const vec<D,A>,B> {using type = const vec<D,B>;};
                template <typename A, typename B> using change_vec_base_t = typename change_vec_base_impl<A,B>::type;

                // Returns a reasonable 'floating-point counterpart' for a type.
                // Currently if the type is not floating-point, returns `double`. Otherwise returns the same type.
                // If `T` is a vector (possibly const), it's base type is changed according to the same rules.
                template <typename T> using floating_point_t = std::conditional_t<std::is_floating_point_v<vec_base_t<T>>, T, change_vec_base_t<T, double>>;

                // 3-way compares two scalar or vector types to determine which one is 'larger' (according to `sizeof`),
                // except floating-point types are always considered to be larger than integral ones.
                // For vector types, examines their base types instead.
                // Returns 0 if the types are same or not comparable.
                template <typename A, typename B> inline constexpr int compare_types_v =
                $   (!is_scalar_v<A> && !is_vector_v<A>) || (!is_scalar_v<B> && !is_vector_v<B>) ? 0 :
                $   std::is_floating_point_v<vec_base_t<A>> < std::is_floating_point_v<vec_base_t<B>> ? -1 :
                $   std::is_floating_point_v<vec_base_t<A>> > std::is_floating_point_v<vec_base_t<B>> ?  1 :
                $   sizeof(vec_base_t<A>)                   < sizeof(vec_base_t<B>)                   ? -1 :
                $   sizeof(vec_base_t<A>)                   > sizeof(vec_base_t<B>)                   ?  1 : 0;

                // Internal, see below for the public interface.
                // Given a list of scalar and vector types, determines the "larger' type among them according to `compare_types_v`.
                // Returns `void` on failure.
                // If vector types are present, all of them must have the same size, and the resulting type will also be a vector.
                template <typename ...P> struct larger_impl {using type = void;};
                template <typename T> struct larger_impl<T> {using type = T;};
                template <typename T, typename ...P> struct larger_impl<T,P...> {using type = typename larger_impl<T, typename larger_impl<P...>::type>::type;};
                template <typename A, typename B> struct larger_impl<A,B> {using type = std::conditional_t<compare_types_v<A,B> != 0, std::conditional_t<(compare_types_v<A,B> > 0), A, B>, std::conditional_t<std::is_same_v<A,B>, A, void>>;};
                template <int D, typename A, typename B> struct larger_impl<vec<D,A>,B> {using type = std::conditional_t<std::is_void_v<typename larger_impl<A,B>::type>, void, change_vec_base_t<vec<D,A>, typename larger_impl<A,B>::type>>;};
                template <int D, typename A, typename B> struct larger_impl<B,vec<D,A>> {using type = std::conditional_t<std::is_void_v<typename larger_impl<A,B>::type>, void, change_vec_base_t<vec<D,A>, typename larger_impl<A,B>::type>>;};
                template <int DA, int DB, typename A, typename B> struct larger_impl<vec<DA,A>,vec<DB,B>>
                {using type = std::conditional_t<DA != DB || std::is_void_v<typename larger_impl<A,B>::type>, void, change_vec_base_t<vec<DA,A>, typename larger_impl<A,B>::type>>;};

                // Returns the 'larger' type among `P` or `void` on failure.
                template <typename ...P> struct opt_larger_impl {using type = typename larger_impl<std::remove_const_t<P>...>::type;};
                template <typename ...P> using opt_larger_t = typename opt_larger_impl<P...>::type; // void on failure

                // Checks if it's possible to determine the 'larger' type among `P`.
                template <typename ...P> inline constexpr bool have_larger_type_v = !std::is_void_v<opt_larger_t<P...>>;

                // Returns the 'larger' type among `P` or causes a SFINAE-friendly error on failure.
                template <typename ...P> using soft_larger_t = std::enable_if_t<have_larger_type_v<P...>, opt_larger_t<P...>>;

                template <typename ...P> struct hard_larger_impl
                {
                    static_assert(have_larger_type_v<P...>, "Can't determine larger type.");
                    using type = opt_larger_t<P...>;
                };

                // Returns the 'larger' type among `P` or triggers a static assertion on failure.
                template <typename ...P> using larger_t = typename hard_larger_impl<P...>::type;
            )");
        });

        next_line();

        section("inline namespace Vector // Definitions", []
        {
            decorative_section("Vectors", [&]
            {
                for (int w = 2; w <= 4; w++)
                {
                    if (w != 2)
                        next_line();

                    section_sc(make_str("template <typename T> struct vec<",w,",T> // vec",w), [&]
                    {
                        auto Fields = [&](std::string fold_op, std::string pre = "", std::string post = "") -> std::string
                        {
                            std::string ret;
                            for (int i = 0; i < w; i++)
                            {
                                if (i != 0)
                                    ret += fold_op;
                                ret += pre + data::fields[i] + post;
                            }
                            return ret;
                        };

                        { // Static assertions
                            output("static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>, \"The base type must have no cv-qualifiers.\");\n");
                            output("static_assert(!std::is_reference_v<T>, \"The base type must not be a reference.\");\n");
                        }

                        { // Aliases
                            output("using type = T;\n");
                        }

                        { // Properties
                            output("static constexpr int size = ",w,";\n");
                            output("static constexpr bool is_floating_point = std::is_floating_point_v<type>;\n");
                        }

                        { // Members
                            for (int i = 0; i < w; i++)
                            {
                                output("union {type ");
                                for (int j = 0; j < data::fields_alt_count; j++)
                                {
                                    if (j != 0)
                                        output(", ");
                                    output(data::fields_alt[j][i]);
                                }
                                output(";};\n");
                            }
                        }

                        { // Constructors
                            // Default
                            output("constexpr vec() = default;\n");

                            // Element-wise
                            output("constexpr vec(",Fields(", ","type "),") : ");
                            for (int i = 0; i < w; i++)
                            {
                                if (i != 0)
                                    output(", ");
                                output(data::fields[i],"(",data::fields[i],")");
                            }
                            output(" {}\n");

                            // Fill with a single value
                            output("explicit constexpr vec(type obj) : ",Fields(", ","", "(obj)")," {}\n");

                            // Converting
                            output("template <typename TT> constexpr vec(vec",w,"<TT> obj) : ");
                            for (int i = 0; i < w; i++)
                            {
                                if (i != 0)
                                    output(", ");
                                output(data::fields[i],"(obj.",data::fields[i],")");
                            }
                            output(" {}\n");
                        }

                        { // Convert to type
                            output("template <typename TT> [[nodiscard]] constexpr vec",w,"<TT> to() const {return vec",w,"<TT>(",Fields(", ", "TT(", ")"),");}\n");
                        }

                        { // Member access
                            // Operator []
                            output("[[nodiscard]] constexpr type &operator[](int i) {return *(type *)((char *)this + sizeof(type)*i);}\n");
                            output("[[nodiscard]] constexpr const type &operator[](int i) const {return *(type *)((char *)this + sizeof(type)*i);}\n");

                            // As array
                            output("[[nodiscard]] type *as_array() {return &x;}\n");
                            output("[[nodiscard]] const type *as_array() const {return &x;}\n");
                        }

                        { // Boolean
                            // Convert to bool
                            output("[[nodiscard]] explicit constexpr operator bool() const {return any(); static_assert(!std::is_same_v<type, bool>, \"Use .none(), .any(), or .all() for vectors of bool.\");}\n");

                            // None of
                            output("[[nodiscard]] constexpr bool none() const {return !any();}\n");

                            // Any of
                            output("[[nodiscard]] constexpr bool any() const {return ",Fields(" || "),";}\n");

                            // All of
                            output("[[nodiscard]] constexpr bool all() const {return ",Fields(" && "),";}\n");
                        }

                        { // Apply operators
                            // Sum
                            output("[[nodiscard]] constexpr auto sum() const {return ", Fields(" + "), ";}\n");

                            // Product
                            output("[[nodiscard]] constexpr auto prod() const {return ", Fields(" * "), ";}\n");

                            // Ratio
                            if (w == 2)
                                output("[[nodiscard]] constexpr auto ratio() const {return ", Fields(" / ","floating_point_t<type>(",")"), ";}\n");

                            // Min
                            output("[[nodiscard]] constexpr type min() const {return std::min({", Fields(","), "});}\n");
                            // Max
                            output("[[nodiscard]] constexpr type max() const {return std::max({", Fields(","), "});}\n");

                            // Abs
                            output("[[nodiscard]] constexpr vec abs() const {return vec(", Fields(", ", "std::abs(", ")"), ");}\n");
                        }

                        { // Resize
                            for (int i = 2; i <= 4; i++)
                            {
                                if (i == w)
                                    continue;
                                output("[[nodiscard]] constexpr vec",i,"<type> to_vec",i,"(");
                                for (int j = w; j < i; j++)
                                {
                                    if (j != w)
                                        output(", ");
                                    output("type n",data::fields[j]);
                                }
                                output(") const {return {");
                                for (int j = 0; j < i; j++)
                                {
                                    if (j != 0)
                                        output(", ");
                                    if (j >= w)
                                        output("n");
                                    output(data::fields[j]);
                                }
                                output("};}\n");
                            }
                            for (int i = w+1; i <= 4; i++)
                            {
                                output("[[nodiscard]] constexpr vec",i,"<type> to_vec",i,"() const {return to_vec",i,"(");
                                for (int j = w; j < i; j++)
                                {
                                    if (j != w)
                                        output(", ");
                                    output("01"[j == 3]);
                                }
                                output(");}\n");
                            }
                        }

                        { // Length and normalization
                            // Squared length
                            output("[[nodiscard]] constexpr auto len_sqr() const {return ");
                            for (int i = 0; i < w; i++)
                            {
                                if (i != 0)
                                    output(" + ");
                                output(data::fields[i],"*",data::fields[i]);
                            }
                            output(";}\n");

                            // Length
                            output("[[nodiscard]] constexpr auto len() const {return std::sqrt(len_sqr());}\n");

                            // Normalize
                            output("[[nodiscard]] constexpr auto norm() const -> vec",w,"<decltype(type{}/len())> {if (auto l = len()) return *this / l; else return vec(0);}\n");
                        }

                        { // Angles and directions
                            if (w == 2)
                            {
                                // Construct from angle
                                output("[[nodiscard]] static constexpr vec dir(type angle, type len = 1) {return vec(std::cos(angle) * len, std::sin(angle) * len); static_assert(is_floating_point, \"The vector must be floating-point.\");}\n");

                                // Get angle
                                output("template <typename TT = double> [[nodiscard]] constexpr TT angle() const {return std::atan2(TT(y), TT(x));}\n"); // Note that atan2 is well-defined even when applied to (0,0).

                                // Rotate by 90 degree increments
                                output("[[nodiscard]] constexpr vec rot90(int steps = 1) const {switch (steps & 3) {default: return *this; case 1: return {-y,x}; case 2: return -*this; case 3: return {y,-x};}}\n");

                                // Return one of the 4 main directions, `vec2(1,0).rot90(index)`
                                output("[[nodiscard]] static constexpr vec dir4(int index) {return vec(1,0).rot90(index);}\n");

                                // Return one of the 8 main directions (including diagonals).
                                output("[[nodiscard]] static constexpr vec dir8(int index) {vec array[8]{vec(1,0),vec(1,1),vec(0,1),vec(-1,1),vec(-1,0),vec(-1,-1),vec(0,-1),vec(1,-1)}; return array[index & 7];}\n");
                            }
                        }

                        { // Dot and cross products
                            // Dot product
                            output("template <typename TT> [[nodiscard]] constexpr auto dot(const vec",w,"<TT> &o) const {return ");
                            for (int i = 0; i < w; i++)
                            {
                                if (i != 0)
                                    output(" + ");
                                output(data::fields[i]," * o.",data::fields[i]);
                            }
                            output(";}\n");

                            // Cross product
                            if (w == 3)
                                output("template <typename TT> [[nodiscard]] constexpr auto cross(const vec3<TT> &o) const -> vec3<decltype(x * o.x - x * o.x)> {return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};}\n");

                            // Cross product z component
                            if (w == 2)
                                output("template <typename TT> [[nodiscard]] constexpr auto cross(const vec2<TT> &o) const {return x * o.y - y * o.x;}\n");

                            // Delta_to (aka inverse minus)
                            output("template <typename TT> [[nodiscard]] constexpr auto delta_to(vec",w,"<TT> v) const {return v - *this;}\n");
                        }

                        { // Tie
                            output("[[nodiscard]] constexpr auto tie() {return std::tie(",Fields(","),");}\n");
                            output("[[nodiscard]] constexpr auto tie() const {return std::tie(",Fields(","),");}\n");
                        }

                        { // Get
                            output("template <int I> [[nodiscard]] constexpr auto &get() {return std::get<I>(tie());}\n");
                            output("template <int I> [[nodiscard]] constexpr auto &get() const {return std::get<I>(tie());}\n");
                        }
                    });
                }

                next_line();

                // Deduction guides
                output("template <typename ...P, typename = std::enable_if_t<sizeof...(P) >= 2 && sizeof...(P) <= 4>> vec(P...) -> vec<sizeof...(P), larger_t<P...>>;\n");
            });

            next_line();

            decorative_section("Matrices", [&]
            {
                for (int w = 2; w <= 4; w++)
                for (int h = 2; h <= 4; h++)
                {
                    if (w != 2 || h != 2)
                        next_line();

                    section_sc(make_str("template <typename T> struct mat<",w,",",h,",T> // mat", w, "x", h), [&]
                    {
                        auto LargeFields = [&](std::string fold_op, std::string pre = "", std::string post = "") -> std::string
                        {
                            std::string ret;
                            for (int i = 0; i < w; i++)
                            {
                                if (i != 0)
                                    ret += fold_op;
                                ret += pre + data::fields[i] + post;
                            }
                            return ret;
                        };
                        auto SmallFields = [&](std::string fold_op, std::string pre = "", std::string post = "", std::string mid = ".") -> std::string
                        {
                            std::string ret;
                            for (int y = 0; y < h; y++)
                            for (int x = 0; x < w; x++)
                            {
                                if (x != 0 || y != 0)
                                    ret += fold_op;
                                ret += pre + data::fields[x] + mid + data::fields[y] + post;
                            }
                            return ret;
                        };

                        { // Static assertions
                            output("static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>, \"The base type must have no cv-qualifiers.\");\n");
                            output("static_assert(!std::is_reference_v<T>, \"The base type must not be a reference.\");\n");
                        }

                        { // Aliases
                            output("using type = T;\n");
                            output("using member_type = vec", h,"<T>;\n");
                        }

                        { // Properties
                            output("static constexpr int width = ",w,", height = ",h,";\n");
                            if (w == h)
                                output("static constexpr int size = ",w,";\n");

                            output("static constexpr bool is_floating_point = std::is_floating_point_v<type>;\n");
                        }

                        { // Members
                            for (int i = 0; i < w; i++)
                            {
                                output("union {member_type ");
                                for (int j = 0; j < data::fields_alt_count; j++)
                                {
                                    if (j != 0)
                                        output(", ");
                                    output(data::fields_alt[j][i]);
                                }
                                output(";};\n");
                            }
                        }

                        { // Constructors
                            // Default
                            output("constexpr mat() : mat(");
                            for (int y = 0; y < h; y++)
                            for (int x = 0; x < w; x++)
                            {
                                if (x || y)
                                    output(",");
                                output("01"[x == y]);
                            }
                            output(") {}\n");

                            // Element-wise
                            output("constexpr mat(",LargeFields(", ","const member_type &"),") : ");
                            for (int i = 0; i < w; i++)
                            {
                                if (i != 0)
                                    output(", ");
                                output(data::fields[i],"(",data::fields[i],")");
                            }
                            output(" {}\n");

                            // Matrix element-wise
                            output("constexpr mat(",SmallFields(", ","type ","",""),") : ");
                            for (int x = 0; x < w; x++)
                            {
                                if (x != 0)
                                    output(", ");
                                output(data::fields[x],"(");
                                for (int y = 0; y < h; y++)
                                {
                                    if (y != 0)
                                        output(",");
                                    output(data::fields[x],data::fields[y]);
                                }
                                output(")");
                            }
                            output(" {}\n");

                            // Converting
                            output("template <typename TT> constexpr mat(const mat",w,"x",h,"<TT> &obj) : ");
                            for (int i = 0; i < w; i++)
                            {
                                if (i != 0)
                                    output(", ");
                                output(data::fields[i],"(obj.",data::fields[i],")");
                            }
                            output(" {}\n");
                        }

                        { // Convert to type
                            output("template <typename TT> [[nodiscard]] constexpr mat",w,"x",h,"<TT> to() const {return mat",w,"x",h,"<TT>(",SmallFields(", ","TT(",")"),");}\n");
                        }

                        { // Member access
                            // Operator []
                            output("[[nodiscard]] constexpr member_type &operator[](int i) {return *(member_type *)((char *)this + sizeof(member_type)*i);}\n");
                            output("[[nodiscard]] constexpr const member_type &operator[](int i) const {return *(member_type *)((char *)this + sizeof(member_type)*i);}\n");

                            // As array
                            output("[[nodiscard]] type *as_array() {return &x.x;}\n");
                            output("[[nodiscard]] const type *as_array() const {return &x.x;}\n");
                        }

                        { // Resize
                            // One-dimensional
                            for (int i = 2; i <= 4; i++)
                            {
                                if (i == w)
                                    continue;
                                output("[[nodiscard]] constexpr mat",i,"x",h,"<type> to_vec",i,"(");
                                for (int j = w; j < i; j++)
                                {
                                    if (j != w)
                                        output(", ");
                                    output("const member_type &n",data::fields[j]);
                                }
                                output(") const {return {");
                                for (int j = 0; j < i; j++)
                                {
                                    if (j != 0)
                                        output(", ");
                                    if (j >= w)
                                        output("n");
                                    output(data::fields[j]);
                                }
                                output("};}\n");
                            }
                            for (int i = w+1; i <= 4; i++)
                            {
                                output("[[nodiscard]] constexpr mat",i,"x",h,"<type> to_vec",i,"() const {return to_vec",i,"(");
                                for (int j = w; j < i; j++)
                                {
                                    if (j != w)
                                        output(", ");
                                    output("{}");
                                }
                                output(");}\n");
                            }

                            // Two-dimensional
                            for (int hhh = 2; hhh <= 4; hhh++)
                            {
                                for (int www = 2; www <= 4; www++)
                                {
                                    if (www == w && hhh == h)
                                        continue;
                                    output("[[nodiscard]] constexpr mat",www,"x",hhh,"<type> to_mat",www,"x",hhh,"() const {return {");
                                    for (int hh = 0; hh < hhh; hh++)
                                    {
                                        for (int ww = 0; ww < www; ww++)
                                        {
                                            if (ww != 0 || hh != 0)
                                                output(",");
                                            if (ww < w && hh < h)
                                                output(data::fields[ww],".",data::fields[hh]);
                                            else
                                                output("01"[ww == hh]);
                                        }
                                    }
                                    output("};}\n");
                                    if (www == hhh)
                                        output("[[nodiscard]] constexpr mat",www,"x",hhh,"<type> to_mat",www,"() const {return to_mat",www,"x",www,"();}\n");
                                }
                            }
                        }

                        { // Transpose
                            output("[[nodiscard]] constexpr mat",h,"x",w,"<T> transpose() const {return {");
                            for (int x = 0; x < w; x++)
                            for (int y = 0; y < h; y++)
                            {
                                if (x != 0 || y != 0)
                                    output(",");
                                output(data::fields[x],".",data::fields[y]);
                            }
                            output("};}\n");
                        }

                        { // Inverse
                            if (w == h)
                            {
                                // NOTE: `ret{}` is used instead of `ret`, because otherwise those functions wouldn't be constexpr due to an uninitialized variable.

                                switch (w)
                                {
                                  case 2:
                                    output(1+R"(
                                        [[nodiscard]] constexpr mat inverse()
                                        {
                                            static_assert(is_floating_point, "This function only makes sense for floating-point matrices.");

                                            mat ret{};

                                            ret.x.x =  y.y;
                                            ret.y.x = -y.x;

                                            type d = x.x * ret.x.x + x.y * ret.y.x;
                                            if (d == 0) return {};
                                            d = 1 / d;
                                            ret.x.x *= d;
                                            ret.y.x *= d;

                                            ret.x.y = (-x.y) * d;
                                            ret.y.y = ( x.x) * d;

                                            return ret;
                                        }
                                    )");
                                    break;
                                  case 3:
                                    output(1+R"(
                                        [[nodiscard]] constexpr mat inverse() const
                                        {
                                            static_assert(is_floating_point, "This function only makes sense for floating-point matrices.");

                                            mat ret{};

                                            ret.x.x =  y.y * z.z - z.y * y.z;
                                            ret.y.x = -y.x * z.z + z.x * y.z;
                                            ret.z.x =  y.x * z.y - z.x * y.y;

                                            type d = x.x * ret.x.x + x.y * ret.y.x + x.z * ret.z.x;
                                            if (d == 0) return {};
                                            d = 1 / d;
                                            ret.x.x *= d;
                                            ret.y.x *= d;
                                            ret.z.x *= d;

                                            ret.x.y = (-x.y * z.z + z.y * x.z) * d;
                                            ret.y.y = ( x.x * z.z - z.x * x.z) * d;
                                            ret.z.y = (-x.x * z.y + z.x * x.y) * d;
                                            ret.x.z = ( x.y * y.z - y.y * x.z) * d;
                                            ret.y.z = (-x.x * y.z + y.x * x.z) * d;
                                            ret.z.z = ( x.x * y.y - y.x * x.y) * d;

                                            return ret;
                                        }
                                    )");
                                    break;
                                  case 4:
                                    output(1+R"(
                                        [[nodiscard]] constexpr mat inverse() const
                                        {
                                            static_assert(is_floating_point, "This function only makes sense for floating-point matrices.");

                                            mat ret;

                                            ret.x.x =  y.y * z.z * w.w - y.y * z.w * w.z - z.y * y.z * w.w + z.y * y.w * w.z + w.y * y.z * z.w - w.y * y.w * z.z;
                                            ret.y.x = -y.x * z.z * w.w + y.x * z.w * w.z + z.x * y.z * w.w - z.x * y.w * w.z - w.x * y.z * z.w + w.x * y.w * z.z;
                                            ret.z.x =  y.x * z.y * w.w - y.x * z.w * w.y - z.x * y.y * w.w + z.x * y.w * w.y + w.x * y.y * z.w - w.x * y.w * z.y;
                                            ret.w.x = -y.x * z.y * w.z + y.x * z.z * w.y + z.x * y.y * w.z - z.x * y.z * w.y - w.x * y.y * z.z + w.x * y.z * z.y;

                                            type d = x.x * ret.x.x + x.y * ret.y.x + x.z * ret.z.x + x.w * ret.w.x;
                                            if (d == 0) return {};
                                            d = 1 / d;
                                            ret.x.x *= d;
                                            ret.y.x *= d;
                                            ret.z.x *= d;
                                            ret.w.x *= d;

                                            ret.x.y = (-x.y * z.z * w.w + x.y * z.w * w.z + z.y * x.z * w.w - z.y * x.w * w.z - w.y * x.z * z.w + w.y * x.w * z.z) * d;
                                            ret.y.y = ( x.x * z.z * w.w - x.x * z.w * w.z - z.x * x.z * w.w + z.x * x.w * w.z + w.x * x.z * z.w - w.x * x.w * z.z) * d;
                                            ret.z.y = (-x.x * z.y * w.w + x.x * z.w * w.y + z.x * x.y * w.w - z.x * x.w * w.y - w.x * x.y * z.w + w.x * x.w * z.y) * d;
                                            ret.w.y = ( x.x * z.y * w.z - x.x * z.z * w.y - z.x * x.y * w.z + z.x * x.z * w.y + w.x * x.y * z.z - w.x * x.z * z.y) * d;
                                            ret.x.z = ( x.y * y.z * w.w - x.y * y.w * w.z - y.y * x.z * w.w + y.y * x.w * w.z + w.y * x.z * y.w - w.y * x.w * y.z) * d;
                                            ret.y.z = (-x.x * y.z * w.w + x.x * y.w * w.z + y.x * x.z * w.w - y.x * x.w * w.z - w.x * x.z * y.w + w.x * x.w * y.z) * d;
                                            ret.z.z = ( x.x * y.y * w.w - x.x * y.w * w.y - y.x * x.y * w.w + y.x * x.w * w.y + w.x * x.y * y.w - w.x * x.w * y.y) * d;
                                            ret.w.z = (-x.x * y.y * w.z + x.x * y.z * w.y + y.x * x.y * w.z - y.x * x.z * w.y - w.x * x.y * y.z + w.x * x.z * y.y) * d;
                                            ret.x.w = (-x.y * y.z * z.w + x.y * y.w * z.z + y.y * x.z * z.w - y.y * x.w * z.z - z.y * x.z * y.w + z.y * x.w * y.z) * d;
                                            ret.y.w = ( x.x * y.z * z.w - x.x * y.w * z.z - y.x * x.z * z.w + y.x * x.w * z.z + z.x * x.z * y.w - z.x * x.w * y.z) * d;
                                            ret.z.w = (-x.x * y.y * z.w + x.x * y.w * z.y + y.x * x.y * z.w - y.x * x.w * z.y - z.x * x.y * y.w + z.x * x.w * y.y) * d;
                                            ret.w.w = ( x.x * y.y * z.z - x.x * y.z * z.y - y.x * x.y * z.z + y.x * x.z * z.y + z.x * x.y * y.z - z.x * x.z * y.y) * d;

                                            return ret;
                                        }
                                    )");
                                    break;
                                }
                            }
                        }

                        { // Matrix presets
                            auto MakePreset = [&](int min_sz, int max_sz, std::string name, std::string params, std::string param_names, std::string body, bool float_only = 1)
                            {
                                if (w != h)
                                    return;

                                if (w == min_sz)
                                {
                                    output("[[nodiscard]] static constexpr mat ",name,"(",params,")\n{\n");
                                    if (float_only)
                                        output("static_assert(is_floating_point, \"This function only makes sense for floating-point matrices.\");\n");
                                    output(body,"}\n");
                                }
                                else if (w >= min_sz && w <= max_sz)
                                {
                                    output("[[nodiscard]] static constexpr mat ",name,"(",params,") {return mat",min_sz,"<T>::",name,"(",param_names,").to_mat",w,"();}\n");
                                }
                            };

                            MakePreset(2, 3, "scale", "vec2<type> v", "v", 1+R"(
                                return { v.x , 0   ,
                                    $    0   , v.y };
                            )", 0);

                            MakePreset(3, 4, "scale", "vec3<type> v", "v", 1+R"(
                                return { v.x , 0   , 0   ,
                                    $    0   , v.y , 0   ,
                                    $    0   , 0   , v.z };
                            )", 0);

                            MakePreset(3, 3, "ortho", "vec2<type> min, vec2<type> max", "min, max", 1+R"(
                                return { 2 / (max.x - min.x) , 0                   , (min.x + max.x) / (min.x - max.x) ,
                                    $    0                   , 2 / (max.y - min.y) , (min.y + max.y) / (min.y - max.y) ,
                                    $    0                   , 0                   , 1                                 };
                            )");

                            MakePreset(4, 4, "ortho", "vec2<type> min, vec2<type> max, type near, type far", "min, max, near, far", 1+R"(
                                return { 2 / (max.x - min.x) , 0                   , 0                , (min.x + max.x) / (min.x - max.x) ,
                                    $    0                   , 2 / (max.y - min.y) , 0                , (min.y + max.y) / (min.y - max.y) ,
                                    $    0                   , 0                   , 2 / (near - far) , (near + far) / (near - far)       ,
                                    $    0                   , 0                   , 0                , 1                                 };
                            )");

                            MakePreset(4, 4, "look_at", "vec3<type> src, vec3<type> dst, vec3<type> local_up", "src, dst, local_up", 1+R"(
                                vec3<type> v3 = (src-dst).norm();
                                vec3<type> v1 = local_up.cross(v3).norm();
                                vec3<type> v2 = v3.cross(v1);
                                return { v1.x , v1.y , v1.z , -src.x*v1.x-src.y*v1.y-src.z*v1.z ,
                                    $    v2.x , v2.y , v2.z , -src.x*v2.x-src.y*v2.y-src.z*v2.z ,
                                    $    v3.x , v3.y , v3.z , -src.x*v3.x-src.y*v3.y-src.z*v3.z ,
                                    $    0    , 0    , 0    , 1                                 };
                            )");

                            MakePreset(3, 3, "translate", "vec2<type> v", "v", 1+R"(
                                return { 1, 0, v.x ,
                                    $    0, 1, v.y ,
                                    $    0, 0, 1   };
                            )", 0);

                            MakePreset(4, 4, "translate", "vec3<type> v", "v", 1+R"(
                                return { 1 , 0 , 0 , v.x ,
                                    $    0 , 1 , 0 , v.y ,
                                    $    0 , 0 , 1 , v.z ,
                                    $    0 , 0 , 0 , 1   };
                            )", 0);

                            MakePreset(2, 3, "rotate", "type angle", "angle", 1+R"(
                                type c = std::cos(angle);
                                type s = std::sin(angle);
                                return { c, -s ,
                                    $    s, c  };
                            )");

                            MakePreset(3, 4, "rotate_with_normalized_axis", "vec3<type> axis, type angle", "axis, angle", 1+R"(
                                type c = std::cos(angle);
                                type s = std::sin(angle);
                                return { axis.x * axis.x * (1 - c) + c          , axis.x * axis.y * (1 - c) - axis.z * s , axis.x * axis.z * (1 - c) + axis.y * s,
                                    $    axis.y * axis.x * (1 - c) + axis.z * s , axis.y * axis.y * (1 - c) + c          , axis.y * axis.z * (1 - c) - axis.x * s,
                                    $    axis.x * axis.z * (1 - c) - axis.y * s , axis.y * axis.z * (1 - c) + axis.x * s , axis.z * axis.z * (1 - c) + c         };
                            )", 0);
                            MakePreset(3, 4, "rotate", "vec3<type> axis, type angle", "axis, angle", 1+R"(
                                return rotate_with_normalized_axis(axis.norm(), angle);
                            )");

                            MakePreset(4, 4, "perspective", "type wh_aspect, type y_fov, type near, type far", "wh_aspect, y_fov, near, far", 1+R"(
                                y_fov = type(1) / std::tan(y_fov / 2);
                                return { y_fov / wh_aspect , 0     , 0                           , 0                             ,
                                    $    0                 , y_fov , 0                           , 0                             ,
                                    $    0                 , 0     , (near + far) / (near - far) , 2 * near * far / (near - far) ,
                                    $    0                 , 0     , -1                          , 0                             };
                            )");
                        }
                    });
                }

                next_line();

                // Deduction guides
                output("template <typename ...P, std::enable_if_t<sizeof...(P) == 4, bool> = true> mat(P...) -> mat<2, 2, larger_t<P...>>;\n");
                output("template <typename ...P, std::enable_if_t<sizeof...(P) == 9, bool> = true> mat(P...) -> mat<3, 3, larger_t<P...>>;\n");
                output("template <typename ...P, std::enable_if_t<sizeof...(P) == 16, bool> = true> mat(P...) -> mat<4, 4, larger_t<P...>>;\n");
            });

            next_line();

            decorative_section("Operators", []
            {
                const std::string
                    ops2[]{"+","-","*","/","%","^","&","|","<<",">>","<",">","<=",">=","==","!="},
                    ops2bool[]{"&&","||"},
                    ops1[]{"~","+","-"},
                    ops1incdec[]{"++","--"},
                    ops1bool[]{"!"},
                    ops2as[]{"+=","-=","*=","/=","%=","^=","&=","|=","<<=",">>="};

                for (int d = 2; d <= 4; d++)
                {
                    if (d != 2)
                        next_line();

                    decorative_section(make_str("vec", d), [&]
                    {
                        for (auto op : ops2)
                        {
                            bool all_of = (op == std::string("==")),
                                 any_of = (op == std::string("!=")),
                                 boolean = all_of || any_of;

                            // vec @ vec
                            output("template <typename A, typename B> [[nodiscard]] constexpr ",(boolean ? "bool" : "auto")," operator",op,"(const vec",d,"<A> &a, const vec",d,"<B> &b)",
                                   (boolean ? "" : make_str(" -> vec",d,"<decltype(a.x ",op," b.x)>"))," {return ",(boolean ? "" : "{"));
                            for (int i = 0; i < d; i++)
                            {
                                if (i != 0)
                                    output(all_of ? " && " :
                                           any_of ? " || " : ", ");
                                output("a.",data::fields[i]," ",op," b.", data::fields[i]);
                            }
                            output((boolean ? "" : "}"),";}\n");

                            // vec @ scalar
                            output("template <typename V, typename S, typename = enable_if_scalar_t<S>> [[nodiscard]] constexpr ",(boolean ? "bool" : "auto")," operator",op,"(const vec",d,"<V> &v, const S &s) {return v ",op," vec",d,"<S>(s);}\n");

                            // scalar @ vec
                            output("template <typename S, typename V, typename = enable_if_scalar_t<S>> [[nodiscard]] constexpr ",(boolean ? "bool" : "auto")," operator",op,"(const S &s, const vec",d,"<V> &v) {return vec",d,"<S>(s) ",op," v;}\n");
                        }

                        for (auto op : ops2bool)
                        {
                            // vec @ vec
                            output("template <typename A, typename B> [[nodiscard]] constexpr bool operator",op,"(const vec",d,"<A> &a, const vec",d,"<B> &b) {return bool(a) ",op," bool(b);}\n");

                            // vec @ any
                            output("template <typename A, typename B> [[nodiscard]] constexpr bool operator",op,"(const vec",d,"<A> &a, const B &b) {return bool(a) ",op," bool(b);}\n");

                            // any @ vec
                            output("template <typename A, typename B> [[nodiscard]] constexpr bool operator",op,"(const A &a, const vec",d,"<B> &b) {return bool(a) ",op," bool(b);}\n");
                        }

                        for (auto op : ops1)
                        {
                            // @ vec
                            output("template <typename T> [[nodiscard]] constexpr auto operator",op,"(const vec",d,"<T> &v) -> vec",d,"<decltype(",op,"v.x)> {return {");
                            for (int i = 0; i < d; i++)
                            {
                                if (i != 0)
                                    output(", ");
                                output(op, "v.", data::fields[i]);
                            }
                            output("};}\n");
                        }

                        for (auto op : ops1bool)
                        {
                            // @ vec
                            output("template <typename T> [[nodiscard]] constexpr bool operator",op,"(const vec",d,"<T> &v) {return ",op,"bool(v);}\n");
                        }

                        for (auto op : ops1incdec)
                        {
                            // @ vec
                            output("template <typename T> constexpr vec",d,"<T> &operator",op,"(vec",d,"<T> &v) {");
                            for (int i = 0; i < d; i++)
                                output(op,"v.",data::fields[i],"; ");
                            output("return v;}\n");

                            // vec @
                            output("template <typename T> constexpr vec",d,"<T> operator",op,"(vec",d,"<T> &v, int) {return {");
                            for (int i = 0; i < d; i++)
                            {
                                if (i != 0)
                                    output(", ");
                                output("v.",data::fields[i],op);
                            }
                            output("};}\n");
                        }

                        for (auto op : ops2as)
                        {
                            // vec @ vec
                            output("template <typename A, typename B> constexpr vec",d,"<A> &operator",op,"(vec",d,"<A> &a, const vec",d,"<B> &b) {");
                            for (int i = 0; i < d; i++)
                                output("a.",data::fields[i]," ",op," b.",data::fields[i],"; ");
                            output("return a;}\n");

                            // vec @ scalar
                            output("template <typename V, typename S, typename = enable_if_scalar_t<S>> constexpr vec",d,"<V> &operator",op,"(vec",d,"<V> &v, const S &s) {return v ",op," vec",d,"<S>(s);}\n");
                        }
                    });
                }

                next_line();

                decorative_section("input/output", [&]
                {
                    output(
                    R"( template <typename A, typename B, int D, typename T> std::basic_ostream<A,B> &operator<<(std::basic_ostream<A,B> &s, const vec<D,T> &v)
                        {
                            s.width(0);
                            s << '[';
                            for (int i = 0; i < D; i++)
                            {
                                if (i != 0)
                                $   s << ',';
                                s << v[i];
                            }
                            s << ']';
                            return s;
                        }
                        template <typename A, typename B, int W, int H, typename T> std::basic_ostream<A,B> &operator<<(std::basic_ostream<A,B> &s, const mat<W,H,T> &v)
                        {
                            s.width(0);
                            s << '[';
                            for (int y = 0; y < H; y++)
                            {
                                if (y != 0)
                                $   s << ';';
                                for (int x = 0; x < W; x++)
                                {
                                    if (x != 0)
                                    $   s << ',';
                                    s << v[x][y];
                                }
                            }
                            s << ']';
                            return s;
                        }
                        template <typename A, typename B, int D, typename T> std::basic_istream<A,B> &operator>>(std::basic_istream<A,B> &s, vec<D,T> &v)
                        {
                            s.width(0);
                            for (int i = 0; i < D; i++)
                            $   s >> v[i];
                            return s;
                        }
                        template <typename A, typename B, int W, int H, typename T> std::basic_istream<A,B> &operator>>(std::basic_istream<A,B> &s, mat<W,H,T> &v)
                        {
                            s.width(0);
                            for (int y = 0; y < H; y++)
                            for (int x = 0; x < W; x++)
                            $   s >> v[x][y];
                            return s;
                        }
                    )");
                });

                next_line();

                decorative_section("matrix multiplication", [&]
                {
                    auto Matrix = [&](int x, int y, std::string t) -> std::string
                    {
                        if (x == 1 && y == 1)
                            return t;
                        if (x == 1)
                            return make_str("vec",y,"<",t,">");
                        if (y == 1)
                            return make_str("vec",x,"<",t,">");
                        return make_str("mat",x,"x",y,"<",t,">");
                    };
                    auto Field = [&](int x, int y, int w, int h) -> std::string
                    {
                        if (w == 1 && h == 1)
                            return "";
                        if (w == 1)
                            return data::fields[y];
                        if (h == 1)
                            return data::fields[x];
                        return make_str(data::fields[x], ".", data::fields[y]);
                    };

                    for (int w2 = 1; w2 <= 4; w2++)
                    for (int h1 = 1; h1 <= 4; h1++)
                    for (int w1h2 = 2; w1h2 <= 4; w1h2++) // Starting from 1 would generate `vec * vec` templates (outer products), which would conflict with member-wise multiplication.
                    {
                        if (w2 == 1 && h1 == 1) // This disables generation of `vec * vec` templates (dot products), which would conflict with member-wise multiplication.
                            continue;
                        output("template <typename A, typename B> [[nodiscard]] constexpr ",Matrix(w2,h1,"larger_t<A,B>")," operator*(const ",Matrix(w1h2,h1,"A")," &a, const ",Matrix(w2,w1h2,"B")," &b) {return {");
                        for (int y = 0; y < h1; y++)
                        for (int x = 0; x < w2; x++)
                        {
                            if (y != 0 || x != 0)
                                output(", ");
                            for (int j = 0; j < w1h2; j++)
                            {
                                if (j != 0)
                                    output(" + ");
                                output("a.",Field(j,y,w1h2,h1),"*b.",Field(x,j,w2,w1h2));
                            }
                        }
                        output("};}\n");
                    }

                    next_line();

                    // Only in those two cases return type matches the type of the first parameter.
                    output("template <typename A, typename B, int D> constexpr vec<D,A> &operator*=(vec<D,A> &a, const mat<D,D,B> &b) {a = a * b; return a;}\n");
                    output("template <typename A, typename B, int W, int H> constexpr mat<W,H,A> &operator*=(mat<W,H,A> &a, const mat<W,W,B> &b) {a = a * b; return a;}\n"); // `mat<W,W,B>` is not a typo!
                });
            });
        });

        next_line();

        section("inline namespace Utility // Low-level helper functions", []
        {
            decorative_section("Member access", []
            {
                output(1+R"(
                    // Returns I-th vector element. This function considers scalars to be 1-element vectors.
                    // Returns a non-const reference only if the parameter is a non-const lvalue; otherwise returns a const reference.
                    template <int I, typename T> constexpr auto &get_vec_element(T &&vec)
                    {
                        static_assert(I >= 0 && I < 4);
                        constexpr bool not_const = std::is_reference_v<T> && !std::is_const_v<std::remove_reference_t<T>>;
                        if constexpr (!is_vector_v<std::remove_reference_t<T>>)
                        $   return std::conditional_t<not_const, T &, const T &>(vec);
                        else
                        $   return std::conditional_t<not_const, vec_base_t<std::remove_reference_t<T>> &, const vec_base_t<std::remove_reference_t<T>> &>(vec.template get<I>());
                    }

                    // A simple constexpr `for` loop.
                    template <int D, typename F> constexpr void cexpr_for(F &&func)
                    {
                        static_assert(D >= 1 && D <= 4);
                )");
                for (int i = 0; i < 4; i++)
                {
                    if (i >= 1)
                        output("if constexpr (D > ",i,") ");
                    output("func(std::integral_constant<int,",i,">{});\n");
                }
                output(1+R"(
                    }
                )");
            });

            next_line();

            decorative_section("Custom operators", []
            {
                for (auto op : data::custom_operator_list)
                    output("struct op_type_",op," {using disable_vec_mat_operators = void;};\n");

                next_line();

                for (auto op : data::custom_operator_list)
                {
                    output(1+R"(
                        template <typename A> struct op_expr_type_)",op,R"(
                        {
                            using disable_vec_mat_operators = void;
                            A &&a;
                            template <typename B> [[nodiscard]] constexpr decltype(auto) operator)",data::custom_operator_symbol,R"((B &&b) {return std::forward<A>(a).)",op,R"((std::forward<B>(b));}
                            template <typename B> constexpr decltype(auto) operator)",data::custom_operator_symbol,R"(=(B &&b) {a = std::forward<A>(a).)",op,R"((std::forward<B>(b)); return std::forward<A>(a);}
                        };
                    )");
                }

                next_line();

                for (auto op : data::custom_operator_list)
                    output("template <typename T> inline constexpr op_expr_type_",op,"<T> operator",data::custom_operator_symbol,"(T &&param, op_type_",op,") {return {std::forward<T>(param)};}\n");
            });

            next_line();

            decorative_section("Ranges", []
            {
                output(1+R"(
                    template <typename T> class vector_range
                    {
                        static_assert(is_vector_v<T> && !std::is_const_v<T> && std::is_integral_v<vec_base_t<T>>, "The template parameter must be an integral vector.");

                        T vec_begin = T(0);
                        T vec_end = T(0);

                      @public:
                        using disable_vec_mat_operators = void;

                        class iterator
                        {
                            friend class vector_range<T>;

                            T vec_begin = T(0);
                            T vec_end = T(0);
                            T vec_cur = T(0);
                            bool finished = 1;

                            iterator(T vec_begin, T vec_end) : vec_begin(vec_begin), vec_end(vec_end), vec_cur(vec_begin), finished((vec_begin >= vec_end).any()) {}

                          @public:
                            using difference_type   = std::ptrdiff_t;
                            using value_type        = T;
                            using pointer           = const T *;
                            using reference         = const T &;
                            using iterator_category = std::forward_iterator_tag;

                            iterator() {}

                            iterator &operator++()
                            {
                                bool stop = 0;
                                cexpr_for<vec_size_v<T>>([&](auto index)
                                {
                                    if (stop)
                                    $   return;

                                    constexpr int i = index.value;

                                    auto &elem = get_vec_element<i>(vec_cur);
                                    elem++;
                                    if (elem >= get_vec_element<i>(vec_end))
                                    {
                                        elem = get_vec_element<i>(vec_begin);

                                        if constexpr (i == vec_size_v<T> - 1)
                                        $   finished = 1;
                                    }
                                    else
                                    {
                                        stop = 1;
                                    }
                                });

                                return *this;
                            }
                            iterator operator++(int)
                            {
                                iterator ret = *this;
                                ++(*this);
                                return ret;
                            }

                            reference operator*() const
                            {
                                return vec_cur;
                            }
                            pointer operator->() const
                            {
                                return &vec_cur;
                            }

                            bool operator==(const iterator &other) const
                            {
                                if (finished != other.finished)
                                $   return 0;
                                if (finished && other.finished)
                                $   return 1;
                                return vec_cur == other.vec_cur;
                            }
                            bool operator!=(const iterator &other) const
                            {
                                return !(*this == other);
                            }
                        };

                        vector_range() {}
                        vector_range(T vec_begin, T vec_end) : vec_begin(vec_begin), vec_end(vec_end) {}

                        iterator begin() const
                        {
                            return iterator(vec_begin, vec_end);
                        }

                        iterator end() const
                        {
                            return {};
                        }

                        template <int A, typename B> friend vector_range operator+(const vector_range &range, vec<A,B> offset)
                        {
                            static_assert(std::is_same_v<T, vec<A,B>>, "The offset must have exactly the same type as the range.");
                            return vector_range(range.vec_begin + offset, range.vec_end + offset);
                        }
                        template <int A, typename B> friend vector_range operator+(vec<A,B> offset, const vector_range &range)
                        {
                            return range + offset;
                        }
                    };

                    template <typename T> class vector_range_halfbound
                    {
                        static_assert(is_vector_v<T> && !std::is_const_v<T> && std::is_integral_v<vec_base_t<T>>, "The template parameter must be an integral vector.");

                        T vec_begin = T(0);

                      @public:
                        using disable_vec_mat_operators = void;

                        vector_range_halfbound(T vec_begin) : vec_begin(vec_begin) {}

                        template <int A, typename B> friend vector_range<T> operator<(const vector_range_halfbound &range, vec<A,B> point)
                        {
                            static_assert(std::is_same_v<T, vec<A,B>>, "The upper limit must have exactly the same type as the lower limit.");
                            return vector_range<T>(range.vec_begin, point);
                        }
                        template <int A, typename B> friend vector_range<T> operator<=(const vector_range_halfbound &range, vec<A,B> point)
                        {
                            return range < point+1;
                        }
                    };

                    struct vector_range_factory
                    {
                        using disable_vec_mat_operators = void;

                        template <int A, typename B> vector_range<vec<A,B>> operator()(vec<A,B> size) const
                        {
                            return vector_range<vec<A,B>>(vec<A,B>(0), size);
                        }

                        template <int A, typename B> friend vector_range_halfbound<vec<A,B>> operator<=(vec<A,B> point, vector_range_factory)
                        {
                            return {point};
                        }
                        template <int A, typename B> friend vector_range_halfbound<vec<A,B>> operator<(vec<A,B> point, vector_range_factory)
                        {
                            return point+1 <= vector_range_factory{};
                        }
                    };
                )");
            });
        });

        next_line();

        section("inline namespace Common // Common functions", []
        {
            output(1+R"(
                // Named operators.
            )");
            for (auto op : data::custom_operator_list)
                output("inline constexpr op_type_", op, " ", op, ";\n");

            next_line();

            output(1+R"(
                // Helper class for writing nested loops.
                // Example usage:
                //   for (auto v : vec_a <= vector_range <= vec_b) // `<` are also allowed, in one or both positions.
                //   for (auto v : vector_range(vec_a)) // Equivalent to `vec..(0) <= vector_range < vec_a`.
            )");
            output("inline constexpr vector_range_factory vector_range;\n");

            next_line();

            output(1+R"(
                // Helper for applying a function to one or several scalars or vectors.
                // Mixing scalars and vectors is allowed, but vectors must have the same size.
                // If at least one vector is passed, the result is also a vector.
                template <typename F, typename ...P> constexpr auto apply_elementwise(F &&func, P &&... params)
                {
                    using larger_type = opt_larger_t<change_vec_base_t<std::remove_reference_t<P>, int>...>;
                    static_assert(!std::is_void_v<larger_type>, "Parameter size mismatch.");
                    constexpr int size = vec_size_v<larger_type>;

                    using ret_type = decltype(std::declval<F>()(get_vec_element<0>(std::declval<P>())...));

                    if constexpr (std::is_void_v<ret_type>)
                    {
                        cexpr_for<size>([&](auto index)
                        {
                            func(get_vec_element<index.value>(params)...); // No forwarding to prevent moving.
                        });
                        return void();
                    }
                    else
                    {
                        std::conditional_t<size != 1, vec<size, ret_type>, ret_type> ret{};
                        cexpr_for<size>([&](auto index)
                        {
                            get_vec_element<index.value>(ret) = func(get_vec_element<index.value>(params)...); // No forwarding to prevent moving.
                        });
                        return ret;
                    }
                }

                // The value of pi.
                template <typename T> [[nodiscard]] constexpr T pi() {return T(3.14159265358979323846l);}
                constexpr float       f_pi  = pi<float>();
                constexpr double      d_pi  = pi<double>();
                constexpr long double ld_pi = pi<long double>();

                // Conversions between degrees and radians.
                template <typename T> [[nodiscard]] constexpr auto to_rad(T in)
                {
                    using fp_t = floating_point_t<T>;
                    return in * pi<fp_t>() / fp_t(180);
                }
                template <typename T> [[nodiscard]] constexpr auto to_deg(T in)
                {
                    using fp_t = floating_point_t<T>;
                    return in * fp_t(180) / pi<fp_t>();
                }

                // Returns the sign of the argument as `int` or `ivecN`.
                template <typename T> [[nodiscard]] constexpr change_vec_base_t<T,int> sign(T val)
                {
                    // Works on scalars and vectors.
                    return (val > 0) - (val < 0);
                }

                // `clamp[_var][_min|_max|_abs] (value, min, max)`.
                // Clamps scalars or vectors.
                // `_var` functions modify the first parameter instead of returning the result.
                // `_min` functions don't have a `max` parameter, and vice versa.
                // `_abs` functions don't have a `min` parameter, they use `-max` as `min`.
                // If both `min` and `max` are omitted, 0 and 1 are assumed.
                // If bounds contradict each other, only the `max` bound is used.

                template <typename A, typename B> constexpr void clamp_var_min(A &var, B min)
                {
                    static_assert(is_vector_v<B> <= is_vector_v<A>, "If `min` is a vector, `var` has to be a vector as well.");
                    static_assert(std::is_floating_point_v<vec_base_t<B>> <= std::is_floating_point_v<vec_base_t<A>>, "If `min` is a floating-point, `var` has to be floating-point as well.");
                    static_assert(std::is_floating_point_v<vec_base_t<A>> || std::is_signed_v<vec_base_t<A>> == std::is_signed_v<vec_base_t<B>>, "If both arguments are integral, they must have the same signedness.");

                    if constexpr (no_vectors_v<A,B>)
                    {
                        if (var < min)
                        $   var = min;
                    }
                    else
                    {
                        apply_elementwise(clamp_var_min<vec_base_t<A>, vec_base_t<B>>, var, min);
                    }
                }

                template <typename A, typename B> constexpr void clamp_var_max(A &var, B max)
                {
                    static_assert(is_vector_v<B> <= is_vector_v<A>, "If `max` is a vector, `var` has to be a vector as well.");
                    static_assert(std::is_floating_point_v<vec_base_t<B>> <= std::is_floating_point_v<vec_base_t<A>>, "If `max` is a floating-point, `var` has to be floating-point as well.");
                    static_assert(std::is_floating_point_v<vec_base_t<A>> || std::is_signed_v<vec_base_t<A>> == std::is_signed_v<vec_base_t<B>>, "If both arguments are integral, they must have the same signedness.");

                    if constexpr (no_vectors_v<A,B>)
                    {
                        if (var > max)
                        $   var = max;
                    }
                    else
                    {
                        apply_elementwise(clamp_var_max<vec_base_t<A>, vec_base_t<B>>, var, max);
                    }
                }

                template <typename A, typename B, typename C> constexpr void clamp_var(A &var, B min, C max)
                {
                    clamp_var_min(var, min);
                    clamp_var_max(var, max);
                }

                template <typename A, typename B> constexpr void clamp_var_abs(A &var, B abs_max)
                {
                    static_assert(std::is_signed_v<vec_base_t<B>>, "`abs_max` must be signed."); // This allows floating-point types too.
                    clamp_var(var, -abs_max, abs_max);
                }

                template <typename A, typename B> [[nodiscard]] constexpr A clamp_min(A val, B min)
                {
                    clamp_var_min(val, min);
                    return val;
                }

                template <typename A, typename B> [[nodiscard]] constexpr A clamp_max(A val, B max)
                {
                    clamp_var_max(val, max);
                    return val;
                }

                template <typename A, typename B, typename C> [[nodiscard]] constexpr A clamp(A val, B min, C max)
                {
                    clamp_var(val, min, max);
                    return val;
                }

                template <typename A, typename B> [[nodiscard]] constexpr A clamp_abs(A val, B abs_max)
                {
                    clamp_var_abs(val, abs_max);
                    return val;
                }

                template <typename A> [[nodiscard]] constexpr A clamp(A val) {return clamp(val, 0, 1);}
                template <typename A> [[nodiscard]] constexpr A clamp_min(A val) {return clamp_min(val, 0);}
                template <typename A> [[nodiscard]] constexpr A clamp_max(A val) {return clamp_max(val, 1);}
                template <typename A> [[nodiscard]] constexpr A clamp_abs(A val) {return clamp_abs(val, 1);}
                template <typename A> constexpr void clamp_var(A &var) {clamp_var(var, 0, 1);}
                template <typename A> constexpr void clamp_var_min(A &var) {clamp_var_min(var, 0);}
                template <typename A> constexpr void clamp_var_max(A &var) {clamp_var_max(var, 1);}
                template <typename A> constexpr void clamp_var_abs(A &var) {clamp_var_abs(var, 1);}

                // Rounds a floating-point scalar or vector.
                // Returns an integral type (`int` by default).
                template <typename I = int, typename F> [[nodiscard]] change_vec_base_t<F,I> iround(F x)
                {
                    static_assert(std::is_floating_point_v<vec_base_t<F>>, "Argument must be floating-point.");
                    static_assert(std::is_integral_v<I> && std::is_signed_v<I>, "Template argument must be integral and signed.");

                    if constexpr(no_vectors_v<F>)
                    {
                        if constexpr (sizeof (I) <= sizeof (long))
                        $   return std::lround(x);
                        else
                        $   return std::llround(x);
                    }
                    else
                    {
                        return apply_elementwise(iround<I, vec_base_t<F>>, x);
                    }
                }

                // Various useful functions.
                // Some of them are imported from `std` and extended to operate on vectors. Some are custom.

                using std::abs;
                template <typename T, std::nullptr_t = std::enable_if_t<!no_vectors_v<T>, std::nullptr_t>{}>
                [[nodiscard]] T abs(T x)
                {
                    return apply_elementwise([](auto val){return std::abs(val);}, x);
                }

                using std::round;
                template <typename T, std::nullptr_t = std::enable_if_t<!no_vectors_v<T>, std::nullptr_t>{}>
                [[nodiscard]] T round(T x)
                {
                    static_assert(std::is_floating_point_v<vec_base_t<T>>, "Argument must be floating-point.");
                    return apply_elementwise([](auto val){return std::round(val);}, x);
                }

                using std::floor;
                template <typename T, std::nullptr_t = std::enable_if_t<!no_vectors_v<T>, std::nullptr_t>{}>
                [[nodiscard]] T floor(T x)
                {
                    static_assert(std::is_floating_point_v<vec_base_t<T>>, "Argument must be floating-point.");
                    return apply_elementwise([](auto val){return std::floor(val);}, x);
                }

                using std::ceil;
                template <typename T, std::nullptr_t = std::enable_if_t<!no_vectors_v<T>, std::nullptr_t>{}>
                [[nodiscard]] T ceil(T x)
                {
                    static_assert(std::is_floating_point_v<vec_base_t<T>>, "Argument must be floating-point.");
                    return apply_elementwise([](auto val){return std::ceil(val);}, x);
                }

                using std::trunc;
                template <typename T, std::nullptr_t = std::enable_if_t<!no_vectors_v<T>, std::nullptr_t>{}>
                [[nodiscard]] T trunc(T x)
                {
                    static_assert(std::is_floating_point_v<vec_base_t<T>>, "Argument must be floating-point.");
                    return apply_elementwise([](auto val){return std::trunc(val);}, x);
                }

                template <typename T> [[nodiscard]] T frac(T x)
                {
                    static_assert(std::is_floating_point_v<vec_base_t<T>>, "Argument must be floating-point.");

                    if constexpr (no_vectors_v<T>)
                    $   return std::modf(x, 0);
                    else
                    $   return apply_elementwise(frac<vec_base_t<T>>, x);
                }

                using std::nexttoward;
                template <typename A, typename B, std::nullptr_t = std::enable_if_t<!no_vectors_v<A, B>, std::nullptr_t>{}>
                [[nodiscard]] A nexttoward(A value, B target)
                {
                    static_assert(std::is_floating_point_v<vec_base_t<A>>, "The first argument must be floating-point.");
                    static_assert(std::is_constructible_v<A, B &>, "Can't convert the second argument to the type of the one.");
                    return apply_elementwise([](auto a, auto b){return std::nexttoward(a, b);}, value, A(target));
                }

                // Integer division, slightly changed to behave nicely for negative values of the left operand:
                //           i : -4  -3  -2  -1  0  1  2  3  4
                // div_ex(i,2) : -2  -2  -1  -1  0  0  1  1  2
                template <typename A, typename B> [[nodiscard]] constexpr A div_ex(A a, B b)
                {
                    static_assert(is_vector_v<B> <= is_vector_v<A>, "If `b` is a vector, `a` has to be a vector as well.");
                    static_assert(std::is_integral_v<vec_base_t<A>> && std::is_integral_v<vec_base_t<B>>, "Arguments must be integral.");

                    if constexpr (no_vectors_v<A,B>)
                    {
                        if (a >= 0)
                        $   return a / b;
                        else
                        $   return (a + 1) / b - sign(b);
                    }
                    else
                    {
                        return apply_elementwise(div_ex<vec_base_t<A>, vec_base_t<B>>, a, b);
                    }
                }

                // True integral modulo that remains periodic for negative values of the left operand.
                template <typename A, typename B> [[nodiscard]] constexpr A mod_ex(A a, B b)
                {
                    static_assert(is_vector_v<B> <= is_vector_v<A>, "If `b` is a vector, `a` has to be a vector as well.");
                    static_assert(std::is_integral_v<vec_base_t<A>> && std::is_integral_v<vec_base_t<B>>, "Arguments must be integral.");

                    if constexpr (no_vectors_v<A,B>)
                    {
                        if (a >= 0)
                        $   return a % b;
                        else
                        $   return abs(b) - 1 + (a + 1) % b;
                    }
                    else
                    {
                        return apply_elementwise(mod_ex<vec_base_t<A>, vec_base_t<B>>, a, b);
                    }
                }

                // Simple implementation of `pow` for non-negative integral powers.
                template <typename A, typename B> [[nodiscard]] constexpr A ipow(A a, B b)
                {
                    // `A` can be a scalar or a vector. `B` has to be scalar.
                    static_assert(std::is_integral_v<B>, "Power must be integral.");
                    A ret = 1;
                    while (b-- > 0)
                    $   ret *= a;
                    return ret;
                }

                using std::pow;
                template <typename A, typename B, std::nullptr_t = std::enable_if_t<!no_vectors_v<A, B>, std::nullptr_t>{}>
                [[nodiscard]] auto pow(A a, B b)
                {
                    return apply_elementwise([](auto val_a, auto val_b){return std::pow(val_a, val_b);}, a, b);
                }

                // Computes the smooth step function. Doesn't clamp `x`.
                template <typename T> [[nodiscard]] constexpr T smoothstep(T x)
                {
                    // No special handling required for `T` being a vector.
                    static_assert(std::is_floating_point_v<vec_base_t<T>>, "Argument must be floating-point.");
                    return (3 - 2*x) * x*x;
                }

                // Performs linear interpolation. Returns `a * (1-factor) + b * factor`.
                template <typename F, typename T> [[nodiscard]] constexpr auto mix(F factor, T a, T b)
                {
                    static_assert(std::is_floating_point_v<vec_base_t<F>>, "`factor` must be floating-point.");
                    static_assert(!is_vector_v<F> || !is_vector_v<T> || vec_size_v<F> == vec_size_v<T>, "If both `factor` and the values are vectors, they must have the same size.");
                    // No special handling required for the parameters being vectors.
                    return a * (1-factor) + b * factor;
                }

                // Returns a `min` or `max` value of the parameters.
                template <typename ...P> [[nodiscard]] constexpr larger_t<P...> min(P ... params)
                {
                    if constexpr (no_vectors_v<P...>)
                    $   return std::min({larger_t<P...>(params)...});
                    else
                    $   return apply_elementwise(min<vec_base_t<P>...>, params...);
                }
                template <typename ...P> [[nodiscard]] constexpr larger_t<P...> max(P ... params)
                {
                    if constexpr (no_vectors_v<P...>)
                    $   return std::max({larger_t<P...>(params)...});
                    else
                    $   return apply_elementwise(max<vec_base_t<P>...>, params...);
                }
            )");
        });

        next_line();

        section("inline namespace Misc // Misc functions", []
        {
            output(1+R"(
                // A functor that performs linear mapping on scalars or vectors.
                template <typename T> struct linear_mapping
                {
                    static_assert(std::is_floating_point_v<vec_base_t<T>>, "Template parameter must be floating-point.");

                    T scale = T(1), offset = T(0);

                    linear_mapping() = default;

                    linear_mapping(T src_a, T src_b, T dst_a, T dst_b)
                    {
                        T factor = 1 / (src_a - src_b);
                        scale = (dst_a - dst_b) * factor;
                        offset = (dst_b * src_a - dst_a * src_b) * factor;
                    }

                    T operator()(T x) const
                    {
                        return x * scale + offset;
                    }

                    using matrix_t = mat<vec_size_v<T>+1, vec_size_v<T>+1, vec_base_t<T>>;
                    matrix_t matrix() const
                    {
                        matrix_t ret{};
                        for (int i = 0; i < vec_size_v<T>; i++)
                        {
                            ret[i][i] = scale[i];
                            ret[vec_size_v<T>][i] = offset[i];
                        }
                        return ret;
                    }
                };

                // Shrinks a vector as little as possible to give it specific proportions.
                // Always returns a floating-point type.
                template <typename A, typename B> [[nodiscard]] auto shrink_to_proportions(A value, B proportions)
                {
                    static_assert(is_vector_v<A> && is_vector_v<B> && vec_size_v<A> == vec_size_v<B>, "Arguments must be vectors of same size.");
                    using type = larger_t<floating_point_t<A>,floating_point_t<B>>;
                    return (type(value) / type(proportions)).min() * type(proportions);
                }
                // Expands a vector as little as possible to give it specific proportions.
                // Always returns a floating-point type.
                template <typename A, typename B> [[nodiscard]] auto expand_to_proportions(A value, B proportions)
                {
                    static_assert(is_vector_v<A> && is_vector_v<B> && vec_size_v<A> == vec_size_v<B>, "Arguments must be vectors of same size.");
                    using type = larger_t<floating_point_t<A>,floating_point_t<B>>;
                    return (type(value) / type(proportions)).max() * type(proportions);
                }

                // Finds an intersection point of two lines.
                template <typename T> [[nodiscard]] vec2<T> line_intersection(vec2<T> a1, vec2<T> a2, vec2<T> b1, vec2<T> b2)
                {
                    static_assert(std::is_floating_point_v<T>, "Arguments must be floating-point.");
                    auto delta_a = a2 - a1;
                    auto delta_b = b2 - b1;
                    return ((a1.y - b1.y) * delta_b.x - (a1.x - b1.x) * delta_b.y) / (delta_a.x * delta_b.y - delta_a.y * delta_b.x) * delta_a + a1;
                }

                // Projects a point onto a line. `dir` is assumed to be normalized.
                template <int D, typename T> [[nodiscard]] vec<D,T> project_onto_line_norm(vec<D,T> point, vec<D,T> dir)
                {
                    static_assert(std::is_floating_point_v<T>, "Arguments must be floating-point.");
                    return dir * point.dot(dir);
                }
                // Projects a point onto a line.
                template <int D, typename T> [[nodiscard]] vec<D,T> project_onto_line(vec<D,T> point, vec<D,T> dir)
                {
                    return project_onto_line_norm(point, dir.norm());
                }

                // Projects a point onto a plane. `plane_normal` is assumed to be normalized.
                template <typename T> [[nodiscard]] vec3<T> project_onto_plane_norm(vec3<T> point, vec3<T> plane_normal)
                {
                    return point - project_onto_line_norm(point, plane_normal);
                }
                // Projects a point onto a plane.
                template <typename T> [[nodiscard]] vec3<T> project_onto_plane(vec3<T> point, vec3<T> plane_normal)
                {
                    return project_onto_plane_norm(point, plane_normal.norm());
                }
            )");
        });

        next_line();

        section("namespace Export", []
        {
            output(1+R"(
                using Vector::vec; // Vector and matrix definitions. We use this instead of `using namespace Vector` to avoid bringing...
                using Vector::mat; // ...the overloaded operators into the global namespace, mostly for better error messages and build speed.
                using namespace Alias; // Convenient type aliases.
                using namespace Common; // Common functions.

                // Common types.
                using std::int8_t;
                using std::uint8_t;
                using std::int16_t;
                using std::uint16_t;
                using std::int32_t;
                using std::uint32_t;
                using std::int64_t;
                using std::uint64_t;
                using std::size_t;
                using std::ptrdiff_t;
                using std::intptr_t;
                using std::uintptr_t;

                // Common standard functions.
                using std::sqrt;
                using std::cos;
                using std::sin;
                using std::tan;
                using std::acos;
                using std::asin;
                using std::atan;
                using std::atan2;
            )");
        });
    });

    next_line();

    section("namespace std", []
    {
        output(1+R"(
            template <int D, typename T> struct less<Math::vec<D,T>>
            {
                using result_type = bool;
                using first_argument_type = Math::vec<D,T>;
                using second_argument_type = Math::vec<D,T>;
                constexpr bool operator()(const Math::vec<D,T> &a, const Math::vec<D,T> &b) const
                {
                    return a.tie() < b.tie();
                }
            };

            template <int D, typename T> struct hash<Math::vec<D,T>>
            {
                using result_type = std::size_t;
                using argument_type = Math::vec<D,T>;
                std::size_t operator()(const Math::vec<D,T> &v) const
                {
                    std::size_t ret = std::hash<decltype(v.x)>{}(v.x);
                    for (int i = 1; i < D; i++)
                    $   ret ^= std::hash<decltype(v.x)>{}(v[i]) + 0x9e3779b9 + (ret << 6) + (ret >> 2); // From Boost.
                    return ret;
                }
            };
        )");
    });

    next_line();

    output("using namespace Math::Export;\n");

    if (!impl::output_file)
        return -1;
}
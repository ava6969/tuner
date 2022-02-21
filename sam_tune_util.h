
#ifndef SAM_RL_SAM_TUNE_UTIL_H
#define SAM_RL_SAM_TUNE_UTIL_H


#include "random"
#include "yaml-cpp/yaml.h"
#include "boost/algorithm/string.hpp"
#include <typeinfo>

namespace sam_tuner {

    template<class T>
    struct Functor{
        virtual T step() = 0;

        [[nodiscard]] inline virtual size_t size() const{ return 1; }

        virtual void reset() {}

        virtual bool hasNext() const { return true; };
    };

    struct GridSearch : Functor<std::string>{

    private:
        typename std::vector<std::string>::iterator current;
        std::vector <std::string> space;
    public:

        using value = std::string;
        explicit GridSearch(std::vector <std::string> space) : space( std::move(space) ) {
            if (this->space.size() < 2)
                throw std::runtime_error("Grid Search Requires that space has at least 2 values to "
                                         "search through");
            current = this->space.begin();
        }

        std::string step() override {
            auto val = *current;
            current = current == space.end() ? space.begin() : current + 1;
            return val;
        }

        inline size_t size() const override{ return space.size(); }

        void reset() override { current = this->space.begin(); }

        virtual bool hasNext() const override{ return current != this->space.end(); };
    };

    struct Choice : Functor<std::string>{
        using value = std::string;
        std::vector <std::string> space;
        std::random_device rd;

        explicit Choice(std::vector <std::string>  space) : space( std::move(space) ) {
            if (this->space.size() < 2)
                throw std::runtime_error("Choice Requires that space has at least 2 values to search through");
        }

        std::string step() override{
            std::vector <std::string> out(1);
            std::sample(space.begin(), space.end(), std::begin(out), 1, std::mt19937{rd()});
            return out[0];
        }
    };

    template< typename T, typename K>
    struct Dist: Functor<K>{
    protected:
        T distribution;
        std::random_device rd;
        std::mt19937 gen;

    public:
        using value = K;
        Dist(K x, K y):distribution(x, y ),
        rd(),
        gen(rd()) {}

        K step() override{
            return distribution(gen);
        }
    };

    template<typename T, typename K>
    struct LogDist : Dist<T, K> {
        using value = K;
        explicit LogDist( int base, K  x, K  y) : Dist<T, K>( std::log(x) / log(base), log(y) / log(base) ){}
    };

    template<typename T, typename K>
    struct TrueDist : Dist<T, K> {
        using value = K;
        explicit TrueDist(K  x, K  y) : Dist<T, K>(x, y){}
    };

    template<class T, class ... Args>
    struct QDist : Functor<double>{
        T dist;
        const int factor;
        explicit QDist(int dp, Args ... args):dist( std::forward<Args>(args) ...), factor( std::pow(10, dp) ){}
        explicit QDist(int dp, int base, Args ... args):dist( base, std::forward<Args>(args) ...), factor( std::pow(10, dp) ){}
        double step() override{
            return static_cast<double>(static_cast<int>(dist.step() * factor) / factor);
        }
        using value = double;

        [[nodiscard]] size_t size() const override{
            return dist.size();
        }
    };

    template<typename In, typename ... Args>
    struct NodeWrapper : Functor<YAML::Node >{

        In x;

        explicit NodeWrapper(Args ... args):x( std::forward<Args>(args) ... ) {}
        explicit NodeWrapper(int dp, int base, Args ... args):x( dp, base, std::forward<Args>(args) ... ) {}

        inline YAML::Node step() override{
            auto result = x.step();
            if constexpr( std::is_same_v< typename In::value, std::string>  ){
                return YAML::Load( result );
            }else{
                return YAML::Load( std::to_string(result) );
            }
        }

        [[nodiscard]] size_t size() const override{
            return x.size();
        }

        void reset() override { x.reset(); }

        [[nodiscard]] bool hasNext() const override{ return x.hasNext(); }
    };

    using LogUniform = LogDist<std::uniform_real_distribution<double>, double>;
    using LogRandInt = LogDist<std::uniform_int_distribution<int64_t>, int64_t>;
    using RandInt     = TrueDist<std::uniform_int_distribution<int64_t>, int64_t>;
    using Uniform     = TrueDist<std::uniform_real_distribution<double>, double>;
    using RandN       = TrueDist<std::normal_distribution<double>, double>;
    using QLogUniform = QDist<LogUniform, double, double>;
    using QLogRandInt = QDist<LogRandInt, int64_t, int64_t>;
    using QRandInt    = QDist<RandInt, int64_t, int64_t>;
    using QUniform    = QDist<Uniform, double, double>;
    using QRandN      = QDist<RandN, double, double>;

}


#endif //SAM_RL_SAM_TUNE_UTIL_H

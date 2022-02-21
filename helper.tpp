//
// Created by dewe on 12/31/21.
//

#include "glog/logging.h"

#define BOOST_DATE_TIME_NO_LIB

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/asio.hpp>
#include <boost/process.hpp>

namespace sam_tuner{
    using std::vector;
    using std::cout;
    namespace bp = boost::process;
    using namespace boost::interprocess;

    std::filesystem::path cleanupDir(std::string const& envID, OptionalPath const& opt){
        std::filesystem::path savePath = opt.value_or( std::filesystem::current_path() / "log" / envID );
        if(not std::filesystem::exists(savePath))
            std::filesystem::create_directories(savePath);
        else{
            auto path = std::filesystem::current_path() / "log";
            if(not std::filesystem::exists(path))
                std::filesystem::create_directories(path);
            auto files = std::filesystem::directory_iterator(path);
            std::vector< std::filesystem::path> filtered;
            std::copy_if( begin(files), end(files),
                          std::back_inserter(filtered), [&envID](std::filesystem::directory_entry const& file){
                        return boost::starts_with(file.path().filename().string(), envID + "_");
                    });

            std::sort(begin(filtered), end(filtered), [&envID]( std::filesystem::path const& a,
                                                                std::filesystem::path const& b){
                std::string a1=a.filename().string(), b1=b.filename().string();
                boost::replace_all(a1, envID + "_", "" );
                boost::replace_all(b1, envID + "_", "" );
                return std::stoi(a1) < std::stoi(b1);
            });

            if(filtered.empty()){
                savePath = savePath.replace_filename(envID + "_1");
            }else{
                auto fileName = filtered.back().filename().string();
                boost::replace_all(fileName, envID + "_", "");
                auto count = stoi(fileName) + 1;
                savePath = savePath.replace_filename(envID + "_" + std::to_string(count));
            }
            std::filesystem::create_directories(savePath);
        }
        return savePath;
    }

    YAML::Node findChildNode(YAML::Node root, std::vector<std::string>::iterator ptr,
                             std::vector<std::string>::iterator end){
        if(ptr == end){
            return root;
        }
        return findChildNode(root[*ptr], ptr + 1, end);
    }

    void _generate(const YAML::Node &root, const vector<std::string> &keyTree,
                   FunctionNodeMap &key2SearchSpace){

        if(root.IsScalar()){
            auto function = root.as<std::string>( "null" );
            std::string realFunction;
            if( auto space = getSearchSpace(function, realFunction)){
                if(realFunction.find("grid") != std::string::npos)
                    key2SearchSpace.grid.emplace_back(keyTree, std::move(space) );
                else{
                    key2SearchSpace.others.emplace_back(keyTree, std::move(space) );
                }
            }
        }else if(root.IsMap()){
            for(auto node : root) {
                auto copy = keyTree;
                copy.emplace_back( node.first.as<std::string>() );
                _generate(node.second, copy, key2SearchSpace);
            }
        }
    }

    std::vector<YAML::Node> generate(const std::filesystem::path &configPath, int numSamples){
        FunctionNodeMap key2SearchSpace;

        auto root_config = YAML::LoadFile(configPath.string());

        std::cout << "Generating Config Files\n";
        auto start = std::chrono::high_resolution_clock::now();
        _generate(root_config, std::vector<std::string>{} , key2SearchSpace);
        auto elapsed = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count();
        std::cout << "Generated Config Files in " << elapsed << " s\n";

        std::vector<YAML::Node> configs;

        if( key2SearchSpace.grid.empty() and  key2SearchSpace.others.empty()){
            std::cerr << "Trying to generate Configs with not functions\n";
            return {root_config};
        }

        std::cout << "Generating Combinations and Replacing Placeholders\n";
        start = std::chrono::high_resolution_clock::now();
        auto n = numSamples;

        if(not key2SearchSpace.grid.empty()){
            while(numSamples--){
                std::vector<YAML::Node> _configs;
                gridSearchCombination(_configs, key2SearchSpace.grid, root_config);
                searchCombination(_configs, key2SearchSpace.others, root_config);
                std::cout << "Generated Configuration [" << n - numSamples << "/" << n << "]\n";
                configs.insert(configs.end(), _configs.begin(), _configs.end());
            }
        }else if(not key2SearchSpace.others.empty()){
            configs.resize(numSamples);
            for(auto& config: configs){
                config = YAML::Clone(root_config);
                std::cout << "Generated Configuration [" << n - numSamples << "/" << n << "]\n";
            }
            searchCombination(configs, key2SearchSpace.others, root_config);
        }
        elapsed = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count();
        std::cout << "Completed in " << elapsed << " s\n";

        return configs;
    }

    void run(const std::filesystem::path &binary,
             const std::filesystem::path &configPath,
             int numSamples,
             const std::filesystem::path &trialDir,
             std::string const& type) {
        auto configs = generate(configPath, numSamples);
        std::vector<bp::child> handles;

        int worker = 0;
        auto trialDirCleaned = cleanupDir(trialDir.string());
        auto total = configs.size();
        std::vector<std::thread> ths;
        ths.reserve(total);
        std::mutex mtx;
        for(auto const& _config : configs){
            std::stringstream arg_stream;
            auto config = YAML::Clone(_config);
            std::string save_path =  config["save"]["path"].as<std::string>().append("_") + std::to_string(worker);
            config["save"]["path"] = save_path;
            std::filesystem::path  config_path = (trialDirCleaned / "config_").concat(std::to_string(worker))
                    .concat(".yaml");
            auto logfile = (trialDirCleaned / "log_").concat(std::to_string(worker)).concat(".txt");

            arg_stream << binary.string() << " -p " << config_path.string() << " -t " << type;
            auto cmd = arg_stream.str();

            std::ofstream out(config_path);
            out << config;
            out.close();

            worker++;
            ths.emplace_back([cmd, logfile, worker, total, &mtx](){
                {
                    std::lock_guard lk(mtx);
                    std::cout << "launching: " << cmd << " " << worker << "\\" << total << "\n";
                }

                bp::child p(cmd, bp::std_out > bp::null, bp::std_err > stderr, bp::std_in < bp::null);
                p.wait();
                {
                    std::lock_guard lk(mtx);
                    std::cout << "closed: trial " << worker << "\n";
                }
            });
        }

        for(auto& th: ths){
            th.join();
        }
    }

    void searchCombination(vector<YAML::Node> &configs, FNodeMap &other2node, const YAML::Node &main) {
        if(configs.empty())
            configs.emplace_back( YAML::Clone(main) );

        for(auto& config: configs){
            for(auto& item: other2node){
                auto& [key, functor] = item;
                findChildNode(config, key.begin(), key.end()) = functor->step();
            }
        }
    }

    void gridSearchCombination(vector<YAML::Node> &configs, FNodeMap &grid2node, const YAML::Node &main){

        for(auto& item: grid2node){
            auto& [nodePath, functor] = item;
            if(not configs.empty()){
                auto temp_config = configs;
                configs.clear();
                for (auto& node: temp_config) {
                    functor->reset();
                    while (functor->hasNext()){
                        auto new_node = YAML::Clone(node);
                        findChildNode(new_node, nodePath.begin(), nodePath.end()) = functor->step();
                        configs.emplace_back(new_node);
                    }
                }
            }else{
                configs =  std::vector<YAML::Node>{};
                functor->reset();
                while (functor->hasNext()){
                    configs.emplace_back(  YAML::Clone(main) );
                    findChildNode(configs.back(), nodePath.begin(), nodePath.end()) = functor->step();
                }

            }
        }
    }

    template<class SearchType> std::unique_ptr< Functor<YAML::Node> >
            makeSearchTrue(std::vector<std::string> const& args,
                           typename SearchType::value(*fn)(std::string const&, size_t*)){
        auto x = fn(args[1], nullptr);
        auto y = fn(args[2], nullptr);

        return std::make_unique< NodeWrapper< SearchType,
                typename SearchType::value, typename SearchType::value> >(x, y );
    }

    template<class SearchType>
    std::unique_ptr< Functor<YAML::Node>  >
    makeSearchQuantized(std::vector<std::string> const& args,
                        typename SearchType::value(*fn)(std::string const&, size_t*)){

        auto&& x = fn(args[1], nullptr);
        auto&& y = fn(args[2], nullptr);
        int&& dp = std::stoi(args[3]);
        return std::make_unique< NodeWrapper< SearchType, int,
                typename SearchType::value, typename SearchType::value > >(
                std::forward<int>(dp),
                std::forward< typename SearchType::value>(x),
                std::forward< typename SearchType::value>(y) );
    }

    template<class SearchType>
    std::unique_ptr< Functor<YAML::Node>  >
    makeSearchLog(std::vector<std::string> const& args,
                  typename SearchType::value(*fn)(std::string const&, size_t*)){
        auto x = fn(args[1], nullptr);
        auto y = fn(args[2], nullptr);

        return std::make_unique< NodeWrapper< SearchType,
                typename SearchType::value, typename SearchType::value, int> >(std::stoi(args[3]), x, y );

    }

    template<class SearchType>
    std::unique_ptr< Functor<YAML::Node>  >
    makeSearchQuantizedLog(std::vector<std::string> const& args,
                           typename SearchType::value(*fn)(std::string const&, size_t*)){
        typename SearchType::value x = fn(args[1], nullptr);
        typename SearchType::value y = fn(args[2], nullptr);

        return std::make_unique< NodeWrapper< SearchType, typename SearchType::value, typename SearchType::value> >(
                std::forward<int>(std::stoi(args[3])),
                std::forward<int>(std::stoi(args[4])), x, y);
    }

    template<class SearchType>
    std::unique_ptr< Functor<YAML::Node> >
    makeSearch(std::vector<std::string> const& args){
        std::vector<std::string> fargs( args.begin() + 1, args.end());
        return std::make_unique<NodeWrapper< SearchType, decltype(fargs) >>(fargs);
    }

    std::unique_ptr<Functor< YAML::Node > > getSearchSpace(std::string function, std::string& name){

        if(function.empty() or function.substr(0, 2) != "__")
            return nullptr;

        auto opening_bracket = function.find_first_of('(');
        auto closing_bracket = function.find_last_of(')');
        function.replace(opening_bracket, 1, ",");
        function.replace(closing_bracket, 1, "");

        auto toInt = [](std::string const &x, size_t *) -> int64_t {
            return std::stol(x);
        };

        function.erase(std::remove(function.begin(), function.end(), ' '), function.end());
        std::vector<std::string> args;

        boost::split(args, function, boost::is_any_of(","));
        name = args[0];

        if( args[0] == "__grid"){
            return makeSearch<GridSearch>(args);
        }else if( args[0] == "__choice"){
            return makeSearch<Choice>(args);
        }else if( args[0] == "__uniform"){
            return makeSearchTrue<Uniform>(args, std::stod);
        }
        else if( args[0] == "__quniform"){
            return makeSearchQuantized< QUniform >(args, std::stod);
        }
        else if( args[0] == "__qloguniform"){
            return makeSearchQuantizedLog<QLogUniform>(args, std::stod);
        }else if( args[0] == "__loguniform"){
            return makeSearchLog<LogUniform>(args, std::stod);
        }else if( args[0] == "__randn"){
            return makeSearchTrue<RandN>(args, std::stod);
        }else if( args[0] == "__qrandn"){
            return makeSearchQuantized<QRandN>(args, std::stod);
        }else if( args[0] == "__randint"){
            return makeSearchTrue<RandInt>(args, toInt);
        }else if( args[0] == "__qrandint"){
            return makeSearchQuantized<QRandInt>(args, std::stod);
        }else if( args[0] == "__lograndint"){
            return makeSearchLog<LogRandInt>(args, toInt);
        }else if( args[0] == "__qlograndint"){
            return makeSearchQuantizedLog<QLogRandInt>(args, std::stod);
        }

        return nullptr;
    }
}
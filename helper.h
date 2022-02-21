//
// Created by dewe on 12/31/21.
//

#ifndef SAM_RL_HELPER_H
#define SAM_RL_HELPER_H

#include "vector"
#include "string"
#include "sam_tune_util.h"
#include "yaml-cpp/yaml.h"
#include "filesystem"
#include "iostream"
#include "optional"


namespace sam_tuner{

    using FNodeMap = std::vector<std::pair<std::vector<std::string>, std::unique_ptr<Functor<YAML::Node> > > >;
    using OptionalPath = std::optional<std::filesystem::path>;
    using NodeGroup = std::vector<YAML::Node>;
    using StringList = std::vector<std::string> ;

    struct FunctionNodeMap {
        FNodeMap grid{}, others{};
    };

    std::filesystem::path cleanupDir(std::string const& envID, OptionalPath const& opt=std::nullopt);

    void gridSearchCombination(NodeGroup &configs, FNodeMap &grid2node, YAML::Node const &main);

    void searchCombination(NodeGroup &configs, FNodeMap &other2node, YAML::Node const &main);

    void _generate(YAML::Node const &root, StringList const &keyTree, FunctionNodeMap &key2SearchSpace);

    NodeGroup generate(std::filesystem::path const &configPath, int numSamples);

    YAML::Node findChildNode(YAML::Node root, StringList::iterator ptr, StringList::iterator end);

    void run(std::filesystem::path const &binary,
             std::filesystem::path const &configPath,
             int numSamples,
             std::filesystem::path const &trialDir);

    template<class SearchType>
    std::unique_ptr< Functor<YAML::Node>  >
    makeSearchTrue(std::vector<std::string> const& args,
                   typename SearchType::value(*fn)(std::string const&, size_t*));

    template<class SearchType>
    std::unique_ptr< Functor<YAML::Node>  >
    makeSearchQuantized(std::vector<std::string> const& args,
                        typename SearchType::value(*fn)(std::string const&, size_t*));

    template<class SearchType>
    std::unique_ptr< Functor<YAML::Node>  >
    makeSearchLog(std::vector<std::string> const& args,
                  typename SearchType::value(*fn)(std::string const&, size_t*));

    template<class SearchType>
    std::unique_ptr< Functor<YAML::Node>  >
    makeSearchQuantizedLog(std::vector<std::string> const& args,
                           typename SearchType::value(*fn)(std::string const&, size_t*));

    template<class SearchType>
    std::unique_ptr< Functor<YAML::Node> >
    makeSearch(std::vector<std::string> const& args,
               typename SearchType::value(*fn)(std::string const&, size_t*));

    void recursive_replace(YAML::Node& x, YAML::Node const& y);

    std::unique_ptr<Functor< YAML::Node > > getSearchSpace(std::string function, std::string& name);
}

#include "helper.tpp"
#endif //SAM_RL_HELPER_H

//
// Created by dewe on 11/18/21.
//

#include "helper.h"
#include "argparse/argparse.hpp"

int main(int argc, char** argv){

    argparse::ArgumentParser program("SAM: The Reinforcement Learning Framework Tuner");
    program.add_argument("-n", "--num_samples").help("").default_value<int>(1).scan<'d', int>();
    program.add_argument("-p", "--config_path").help("path to main config").required();
    program.add_argument("-t", "--trial_path").help("").required();
    program.add_argument("-b", "--binary_path").help("").required();
    program.add_argument("--type").help("cython_src|atari|{leave empty}").default_value("");

    program.parse_args(argc, argv);

    int num_samples = program.get<int>("--num_samples");

    sam_tuner::run(std::filesystem::path(program.get<std::string>("--binary_path")),
                std::filesystem::path(program.get<std::string>("--config_path")), num_samples,
                std::filesystem::path(program.get<std::string>("--trial_path")),
                program.get<std::string>("--type"));

    return 0;
}
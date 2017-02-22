#include "customize/customizer.hpp"

#include "util/log.hpp"
#include "util/meminfo.hpp"
#include "util/version.hpp"

#include <tbb/task_scheduler_init.h>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <iostream>

using namespace osrm;

enum class return_code : unsigned
{
    ok,
    fail,
    exit
};

return_code
parseArguments(int argc, char *argv[], customize::CustomizationConfig &customization_config)
{
    // declare a group of options that will be allowed only on command line
    boost::program_options::options_description generic_options("Options");
    generic_options.add_options()("version,v", "Show version")("help,h", "Show this help message");

    /*
    // declare a group of options that will be allowed both on command line
    boost::program_options::options_description config_options("Configuration");
    config_options.add_options()
        //
        ("threads,t",
         boost::program_options::value<unsigned int>(&customization_config.requested_num_threads)
             ->default_value(tbb::task_scheduler_init::default_num_threads()),
         "Number of threads to use")
        //
        ("max-cell-size",
         boost::program_options::value<std::size_t>(&customization_config.maximum_cell_size)
             ->default_value(4096),
         "Bisection termination citerion based on cell size")
        //
        ("balance",
         boost::program_options::value<double>(&customization_config.balance)->default_value(1.2),
         "Balance for left and right side in single bisection")
        //
        ("boundary",
         boost::program_options::value<double>(&customization_config.boundary_factor)
             ->default_value(0.25),
         "Percentage of embedded nodes to contract as sources and sinks")
        //
        ("optimizing-cuts",
         boost::program_options::value<std::size_t>(&customization_config.num_optimizing_cuts)
             ->default_value(10),
         "Number of cuts to use for optimizing a single bisection")
        //
        ("small-component-size",
         boost::program_options::value<std::size_t>(&customization_config.small_component_size)
             ->default_value(1000),
         "Number of cuts to use for optimizing a single bisection");
    */

    // hidden options, will be allowed on command line, but will not be
    // shown to the user
    boost::program_options::options_description hidden_options("Hidden options");
    hidden_options.add_options()(
        "input,i",
        boost::program_options::value<boost::filesystem::path>(&customization_config.base_path),
        "Input file in .osrm format");

    // positional option
    boost::program_options::positional_options_description positional_options;
    positional_options.add("input", 1);

    // combine above options for parsing
    boost::program_options::options_description cmdline_options;
    cmdline_options
        .add(generic_options) //.add(config_options)
        .add(hidden_options);

    const auto *executable = argv[0];
    boost::program_options::options_description visible_options(
        boost::filesystem::path(executable).filename().string() + " <input.osrm> [options]");
    visible_options.add(generic_options) //.add(config_options)
        ;

    // parse command line options
    boost::program_options::variables_map option_variables;
    try
    {
        boost::program_options::store(boost::program_options::command_line_parser(argc, argv)
                                          .options(cmdline_options)
                                          .positional(positional_options)
                                          .run(),
                                      option_variables);
    }
    catch (const boost::program_options::error &e)
    {
        util::Log(logERROR) << e.what();
        return return_code::fail;
    }

    if (option_variables.count("version"))
    {
        std::cout << OSRM_VERSION << std::endl;
        return return_code::exit;
    }

    if (option_variables.count("help"))
    {
        std::cout << visible_options;
        return return_code::exit;
    }

    boost::program_options::notify(option_variables);

    if (!option_variables.count("input"))
    {
        std::cout << visible_options;
        return return_code::exit;
    }

    return return_code::ok;
}

int main(int argc, char *argv[]) try
{
    util::LogPolicy::GetInstance().Unmute();
    customize::CustomizationConfig customization_config;

    const auto result = parseArguments(argc, argv, customization_config);

    if (return_code::fail == result)
    {
        return EXIT_FAILURE;
    }

    if (return_code::exit == result)
    {
        return EXIT_SUCCESS;
    }

    // set the default in/output names
    customization_config.UseDefaults();

    if (1 > customization_config.requested_num_threads)
    {
        util::Log(logERROR) << "Number of threads must be 1 or larger";
        return EXIT_FAILURE;
    }

    if (!boost::filesystem::is_regular_file(customization_config.base_path))
    {
        util::Log(logERROR) << "Input file " << customization_config.base_path.string()
                            << " not found!";
        return EXIT_FAILURE;
    }

    tbb::task_scheduler_init init(customization_config.requested_num_threads);

    auto exitcode = customize::Customizer().Run(customization_config);

    util::DumpMemoryStats();

    return exitcode;
}
catch (const std::bad_alloc &e)
{
    util::Log(logERROR) << "[exception] " << e.what();
    util::Log(logERROR) << "Please provide more memory or consider using a larger swapfile";
    return EXIT_FAILURE;
}
#ifdef _WIN32
catch (const std::exception &e)
{
    util::Log(logERROR) << "[exception] " << e.what() << std::endl;
    return EXIT_FAILURE;
}
#endif

#include "stdafx.h"
#include "Rule.hpp"
#include "RuleList.h"
#include "BucketTree.h"
#include "TraceGen.h"
#include <getopt.h>


using std::cout;
using std::endl;
using std::ofstream;

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace fs = boost::filesystem;

/* initalize log  */
void logging_init() {
    fs::create_directory("./log");
    logging::add_file_log
    (
        keywords::file_name = "./log/sample_%N.log",
        keywords::rotation_size = 10 * 1024 * 1024,
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
        keywords::format = "[%TimeStamp%]: %Message%"
    );

    /* set severity  */
    /* logging::core::get()->set_filter
     * (
     *     logging::trivial::severity >= warning
     * ); */
}

void print_usage() {
    std::cerr << "Usage: ./TracePrepare (--config <config_file>) [-h:c:t:r:R]\n"
              << "Example: ./TracePrepare -c ../config/TracePrepare_config.ini \\\n"
              << "                        -r ../metadata/ruleset/acl_8000 -t 8"
              << std::endl;
}

int main(int argc, char* argv[]) {
    /* default config */
    string tgen_para_file;
    string rule_file;
    string print_tree;
    int thres_hard = 8;
    int tup2_or_tup4 = 0;
    bool evolving = false;

    print_tree = string("../metadata/tree_pr.dat");

    int getopt_res;
    while (1) {
        static struct option tracegen_options[] = {
            {"2tup",        no_argument,                &tup2_or_tup4, 1},
            {"4tup",        no_argument,                &tup2_or_tup4, 0},
            {"help",        no_argument,                0, 'h'},
            {"evolving",    no_argument,                0, 'e'},
            {"config",      required_argument,          0, 'c'},
            {"thres",       required_argument,          0, 't'},
            {"rules",       required_argument,          0, 'r'},
            {"ref",         required_argument,          0, 'R'},
            {0,             0,                          0,  0}
        };

        int option_index = 0;

        getopt_res = getopt_long (argc, argv, "hec:t:r:R:",
                                  tracegen_options, &option_index);

        if (getopt_res == -1)
            break;

        switch (getopt_res) {
        case 0:
            if (tracegen_options[option_index].flag != 0)
                break;
        case 'c':
            tgen_para_file = string(optarg);
            break;
        case 'e':
            evolving = true;
            break;
        case 'r':
            rule_file = string(optarg);
            break;
        case 'R':
            print_tree = string(optarg);
            break;
        case 't':
            thres_hard = atoi(optarg);
            break;
        case 'h':
            print_usage();
            return 0;
        case '?':
            print_usage();
            return 0;
        default:
            abort();
        }
    }

    srand (time(NULL));
    logging_init();

    /* apply true for testbed, 2 tuple rule */
    rule_list rList(rule_file, bool(tup2_or_tup4));

    // generate bucket tree
    bucket_tree bTree(rList, thres_hard, bool(tup2_or_tup4));
    // bTree.pre_alloc();
    bTree.print_tree(print_tree);

    /* trace generation */
    tracer tGen(&rList, tgen_para_file);
    tGen.print_setup();

    tGen.hotspot_prepare();
    tGen.pFlow_pruning_gen(evolving);

    //tGen.raw_snapshot("./Packet_File/sample-10-12", 10, 300);
    //tGen.raw_hp_similarity("./Packet_File/sample-10-12", 3600, 30, 120, 20);
    //tGen.parse_pcap_file_mp(557,594);
}

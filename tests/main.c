#include <munit.h>

extern MunitSuite test_registry;
extern MunitSuite test_graph;

int
main(int argc, char** argv) {
	MunitSuite suite = {
		.suites = (MunitSuite[]){
			test_registry,
			test_graph,
			{ 0 },
		},
	};

	return munit_suite_main(&suite, NULL, argc, argv);
}

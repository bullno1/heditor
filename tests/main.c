#include <munit.h>

extern MunitSuite test_registry;

int
main(int argc, char** argv) {
	MunitSuite suite = {
		.suites = (MunitSuite[]){
			test_registry,
			{ 0 },
		},
	};

	return munit_suite_main(&suite, NULL, argc, argv);
}

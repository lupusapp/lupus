#include "include/lupus_application.h"

int main(int argc, char **argv) {
    return g_application_run(G_APPLICATION(lupus_application_new()), argc, argv);
}
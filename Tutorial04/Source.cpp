#include "Source.h"

int main() {
    struct RDMAResource res {};

    createRDMAResource(&res);

    connectQP(&res);

    destroyRDMAResource(&res);

    return 0;
}
#include "fizzbuzz.h"

FizzbuzzPkt::FizzbuzzPkt(const uint32_t seq_nb) {
    this->seq_nb = seq_nb;
    this->src_core = rte_lcore_id();
    sprintf(this->data, "%u", seq_nb);
}


FizzbuzzPkt *FizzbuzzPkt::create(const uint32_t seq_nb) {
    FizzbuzzPkt *pkt = (FizzbuzzPkt *)rte_malloc(
        "FizzbuzzPkt", sizeof(FizzbuzzPkt), 0);
    memset(pkt, 0, sizeof(FizzbuzzPkt));
    pkt->seq_nb = seq_nb;
    pkt->src_core = rte_lcore_id();
    sprintf(pkt->data, "%u", seq_nb);
    return pkt;
}

void FizzbuzzPkt::play() {
    char *fizz = "";
    char *buzz = "";
    bool fizzbuzz = false;
    if (this->seq_nb % FIZZ == 0) {
        fizz = "fizz";
        fizzbuzz = true;
    }
    if (this->seq_nb % BUZZ == 0) {
        buzz = "buzz";
        fizzbuzz = true;
    }
    if (fizzbuzz) {
        sprintf(this->data, "%s%s", fizz, buzz);
    }
}

//
// Created by Martin Steinegger on 2019-07-17.
//

#ifndef CONTERMINATOR_MULTIPLETAXAS_H
#define CONTERMINATOR_MULTIPLETAXAS_H

#include <string>
#include <algorithm>
#include "Util.h"
#include "DBWriter.h"
#include "filterdb.h"
#include "NcbiTaxonomy.h"

class Multipletaxas{
public:
    struct RangEntry {
        unsigned int range;
        unsigned int kingdom;
        unsigned int species;
        unsigned int id;

        RangEntry() {};
        RangEntry(unsigned int range, unsigned int kingdom, unsigned int species, unsigned int id) :
                range(range), kingdom(kingdom), species(species), id(id) {}

        bool operator()(const RangEntry &lhs, const RangEntry &rhs) {
            if (lhs.range < rhs.range) return true;
            if (rhs.range < lhs.range) return false;
            if (lhs.kingdom < rhs.kingdom) return true;
            if (rhs.kingdom < lhs.kingdom) return false;
            if (lhs.species < rhs.species) return true;
            if (rhs.species < lhs.species) return false;
            if (lhs.id < rhs.id) return true;
            return false;
        }
    };

    struct TaxonInformation{
        TaxonInformation(int currTaxa, int ancestorTax, int start, int end, char * data) :
                currTaxa(currTaxa), ancestorTax(ancestorTax), start(start), end(end), data(data), overlaps(false), range(-1){}
        int currTaxa;
        int ancestorTax;
        int start;
        int end;
        char * data;
        bool overlaps;
        int range;
        static bool compareByRange(const TaxonInformation &first, const TaxonInformation &second) {
            if (first.range < second.range)
                return true;
            if (second.range < first.range)
                return false;
            if(first.ancestorTax < second.ancestorTax )
                return true;
            if(second.ancestorTax < first.ancestorTax )
                return false;
            if(first.currTaxa < second.currTaxa )
                return true;
            if(second.currTaxa < first.currTaxa )
                return false;
            return false;
        }

        static bool compareByTaxAndStart(const TaxonInformation &first, const TaxonInformation &second) {
            if (first.ancestorTax < second.ancestorTax)
                return true;
            if (second.ancestorTax < first.ancestorTax)
                return false;
            if(first.start < second.start )
                return true;
            if(second.start < first.start )
                return false;
            if(first.currTaxa < second.currTaxa )
                return true;
            if(second.currTaxa < first.currTaxa )
                return false;
            return false;
        }
    };

    static unsigned int getTaxon(unsigned int id, std::vector<std::pair<unsigned int, unsigned int>> &mapping) {
        std::pair<unsigned int, unsigned int> val;
        val.first = id;
        std::vector<std::pair<unsigned int, unsigned int>>::iterator mappingIt = std::upper_bound(
                mapping.begin(), mapping.end(), val, ffindexFilter::compareToFirstInt);
        if (mappingIt->first != val.first) {
            return UINT_MAX;
        }
        return  mappingIt->second;
    }

    static std::vector<TaxonInformation> assignTaxonomy(std::vector<TaxonInformation>  &elements,
                                                        char *data,
                                                        std::vector<std::pair<unsigned int, unsigned int>> & mapping,
                                                        NcbiTaxonomy & t,
                                                        std::vector<int> &taxonList,
                                                        std::vector<int> &blacklist,
                                                        size_t * taxaCounter) {
        elements.clear();
        const char * entry[255];
        while (*data != '\0') {

            bool isAncestor = false;
            const size_t columns = Util::getWordsOfLine(data, entry, 255);
            if (columns == 0) {
                data = Util::skipLine(data);
                continue;
            }
            unsigned int id = Util::fast_atoi<unsigned int>(entry[0]);
            unsigned int taxon = getTaxon(id, mapping);;
            if (taxon == 0 || taxon == UINT_MAX) {
                goto next;
            }
            // remove blacklisted taxa
            for (size_t j = 0; j < blacklist.size(); ++j) {
                if (t.IsAncestor(blacklist[j], taxon)) {
                    goto next;
                }
            }

            size_t j;
            for (j = 0; j < taxonList.size(); ++j) {
                bool isTaxaAncestor = t.IsAncestor(taxonList[j], taxon);
                taxaCounter[j] += isTaxaAncestor;
                isAncestor = std::max(isAncestor, isTaxaAncestor);
                if (isAncestor == true) {
                    break;
                }
            }
            if (isAncestor) {
                int startPos = Util::fast_atoi<int>(entry[4]);
                int endPos   = Util::fast_atoi<int>(entry[5]);
                elements.push_back(TaxonInformation(taxon, taxonList[j], std::min(startPos, endPos), std::max(startPos, endPos), data));
            } else {
                elements.push_back(TaxonInformation(taxon, 0, -1, -1, data));
            }
            next:
            data = Util::skipLine(data);
        }
        return elements;
    }
};

#endif //CONTERMINATOR_MULTIPLETAXAS_H

#include <stdint.h>
#include <stdlib.h>
#include "neske.h"

struct imap *imap_mk()
{
    struct imap *result = calloc(sizeof (struct imap), 1);

    return result;
}

void imap_populate(struct imap *imap, struct ricoh_decoder *decoder, struct ricoh_mem_interface *mem, uint16_t entry)
{
    if (imap->addr[entry]) 
    {
        return;
    }
    
    struct instr_decoded decoded = ricoh_decode_instr(decoder, mem, entry);
    struct print_instr *prev = NULL;

    while (decoded.id != RTS && decoded.id != RTI && decoded.id != BRK)
    {
        struct print_instr *pi = calloc(1, sizeof(struct print_instr));
        ricoh_format_decoded_instr(pi->value, decoded);
        pi->at = entry;
        imap->addr[entry] = pi;

        if (prev)
        {
            prev->next = pi;
            pi->prev = prev;
        }

        if (decoded.id == JSR)
        {
            imap_populate(imap, decoder, mem, *(uint16_t*)decoded.operand);
        }

        entry += decoded.size;
        decoded = ricoh_decode_instr(decoder, mem, entry);
        prev = pi;
    }
}

void imap_list_range(struct imap *imap, uint16_t entry, struct print_instr **dest, int from, int to)
{
    int size = -from + to;

    for (int i = 0; i < size; i++)
    {
        dest[i] = NULL;
    }
    
    if (imap->addr[entry] == NULL)
    {
        return;
    }


    struct print_instr *pi = imap->addr[entry];

    while (pi->prev && from != 0)
    {
        from++;
        pi = pi->prev;
    }

    int start = -from;

    for (int i = start; pi && i < size; i++)
    {
        dest[i] = pi;
        pi = pi->next;
    } 
}
#include "font.h"

const unsigned long long FONT_GLYPHS[128] = {
    [' ']=0,
    ['+' ]=0x0018187E18180000ULL, ['-']=0x000000FE00000000ULL,
    ['.']=0x0000000000181800ULL,  ['/']=0x060C183060C08000ULL,
    ['%']=0xC6CC183066C60000ULL,
    ['0']=0x7CC6CEDEF6E67C00ULL,  ['1']=0x1838181818187E00ULL,
    ['2']=0x7CC6060C3060FE00ULL,  ['3']=0x7CC6061C06C67C00ULL,
    ['4']=0x0C1C3C6CFE0C0C00ULL,  ['5']=0xFEC0FC0606C67C00ULL,
    ['6']=0x3C60C0FCC6C67C00ULL,  ['7']=0xFE06060C18181800ULL,
    ['8']=0x7CC6C67CC6C67C00ULL,  ['9']=0x7CC6C67E06067C00ULL,
    [':']=0x0018180018180000ULL,  ['(']=0x0C18303030180C00ULL,
    [')']=0x30180C0C0C183000ULL,  ['!']=0x1818181818001800ULL,
    ['*']=0x0066663CFF3C6600ULL,
    ['[']=0x3C30303030303C00ULL,  [']']=0x3C0C0C0C0C0C3C00ULL,
    ['A']=0x386CC6FEC6C6C600ULL,  ['B']=0xFC66667C6666FC00ULL,
    ['C']=0x3C66C0C0C0663C00ULL,  ['D']=0xF86C6666666CF800ULL,
    ['E']=0xFE6268786862FE00ULL,  ['F']=0xFE6268786860F000ULL,
    ['G']=0x3C66C0C0CE663E00ULL,  ['H']=0xC6C6C6FEC6C6C600ULL,
    ['I']=0x3C18181818183C00ULL,  ['J']=0x1E0C0C0CCCCC7800ULL,
    ['K']=0xC6CCD8F0D8CCC600ULL,  ['L']=0xF06060606266FE00ULL,
    ['M']=0xC6EEFEFED6C6C600ULL,  ['N']=0xC6E6F6DECEC6C600ULL,
    ['O']=0x7CC6C6C6C6C67C00ULL,  ['P']=0xFC66667C6060F000ULL,
    ['Q']=0x7CC6C6C6D6DE7C06ULL,  ['R']=0xFC66667C6C66F200ULL,
    ['S']=0x7CC6C07C06C67C00ULL,  ['T']=0x7E5A181818183C00ULL,
    ['U']=0xC6C6C6C6C6C67C00ULL,  ['V']=0xC6C6C6C66C381000ULL,
    ['W']=0xC6C6D6FEEEC6C600ULL,  ['X']=0xC66C38386CC6C600ULL,
    ['Y']=0x6666663C18183C00ULL,  ['Z']=0xFE860C183062FE00ULL,
    ['a']=0x0000780C7CCC7600ULL,  ['b']=0xE060607C6666DC00ULL,
    ['c']=0x00007CC6C0C67C00ULL,  ['d']=0x1C0C0C7CCCCC7600ULL,
    ['e']=0x00007CC6FEC07C00ULL,  ['f']=0x1C3630FC30303000ULL,
    ['g']=0x000076CCCC7C0CF8ULL,  ['h']=0xE0606C766666E600ULL,
    ['i']=0x1800381818183C00ULL,  ['j']=0x0600060606C67C00ULL,
    ['k']=0xE060666C786CE600ULL,  ['l']=0x3818181818183C00ULL,
    ['m']=0x0000ECFED6D6C600ULL,  ['n']=0x0000DC6666666600ULL,
    ['o']=0x00007CC6C6C67C00ULL,  ['p']=0x0000DC667C60F000ULL,
    ['q']=0x000076CC7C0C1E00ULL,  ['r']=0x0000DC7660606000ULL,
    ['s']=0x00007CC07C06FC00ULL,  ['t']=0x1030FC3030361C00ULL,
    ['u']=0x0000CCCCCCCC7600ULL,  ['v']=0x0000C6C66C381000ULL,
    ['w']=0x0000C6D6FEEE4400ULL,  ['x']=0x0000C66C386CC600ULL,
    ['y']=0x0000C6C67E060CFCULL,  ['z']=0x0000FC983064FC00ULL,
    [',']=0x0000000000181830ULL,  ['?']=0x7CC60C1818001800ULL,
};

void draw_char(float px, float py, float size, char ch) {
    unsigned char uch = (unsigned char)ch;
    if(uch >= 128) return;
    unsigned long long g = FONT_GLYPHS[uch];
    if(!g && ch != ' ') return;

    for(int row=0;row<8;row++) {
        unsigned char bits = (unsigned char)((g >> (56 - row*8)) & 0xFF);
        for(int col=0;col<8;col++) {
            if(bits & (0x80 >> col)) {
                float x = px + col*size;
                float y = py + (7-row)*size;
                glBegin(GL_QUADS);
                glVertex2f(x,      y);
                glVertex2f(x+size, y);
                glVertex2f(x+size, y+size);
                glVertex2f(x,      y+size);
                glEnd();
            }
        }
    }
}

void draw_text(float x, float y, float size, const char *text) {
    for(int i=0;text[i];i++)
        draw_char(x + i*size*9, y, size, text[i]);
}

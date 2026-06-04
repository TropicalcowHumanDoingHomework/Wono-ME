#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "sim_config.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>

// ============================================================
// Font array definitions
// ============================================================
// PC simulator: U8G2_FONT_SECTION is a no-op (no PROGMEM needed)
#define U8G2_FONT_SECTION(x)

// ============================================================
// Font array definitions (from real U8g2 library)
// ============================================================
const uint8_t u8g2_font_helvB24_tr[2965] U8G2_FONT_SECTION("u8g2_font_helvB24_tr") = 
  "_\0\4\4\5\6\3\6\7\36 \0\371\31\371\31\372\3\314\10\65\13x \6\0`\230\4!\15%"
  ";\270\204\37\250\30\62\337>P\42\17+\61\14\5\62\370\15\11QBD\211\0#\77\62\343\67\65\62"
  "\3\311\14\34Cp\14\71\62\344\310\14$\63\350\201\210\7\42\36\210x \210\314\300A\3\7\15\34C"
  "\350\201\210\7\42\36\210x h\14\71\62\344\310\14$\63p\14\301\61\304\0$\60\220k'=\341b"
  "\221\261qaBD%B\322\10I#&\211\270\204\11\265D\230NH\71\61h\304\240\21\242DH\11"
  "'n\26\32\26.\35\0%G\372*\330\245\261\2\17\216K'p\314\260q\243F\11\34\65h\340\250"
  "\61#G\215\31:f\332\64\242\317\214\37%j\374\230\323b\322\216\231V\314\250\221cF\215\24\64j"
  "\340\240Q\343\206\215\31\70,\335\300\203b\7\1&\65\64\63\210\265\322IW\256$C\260\14\301\62$"
  "\213\220T\233\26\361Y\65D\326\20\251dH\31%\244\222\20;C\256P\271\62\206\320<\30\302\242P"
  "\232RE\1'\13$\61\214\204\7-\204\210\0(\35\350\253\266\254Ad\10\221!Dh\20m\10\321"
  "\37\25\242+B\244F\221\32Ej\0)\37\350\253\266\204Q\24\221\42Dj\24!\252\10\321\337\224!"
  "D\321 \212\6\21\32Dh\24\0*\30ji\333$\201\242\202\10\11\61BD\13D\244\314 \231&"
  "P\20\0+\20\20*\70\65\302\364\354\301\17\210\21\246g\0,\16e\261\226\204\7j$\31\61I "
  "\0-\7\251\350\271\204\37.\10\245\60\230\204\7\12/\32(#\230\64\271\22&\325(aR\215\22&"
  "\225\60\251F\11\223J\230d\0\60\35\17+(\245s\251\326\64)S\242\224V\351\360w\251\264*Q"
  "\246H\233U\351\16\1\61\20\352\62(\275a\244\312<xp\214\376\377\0\62\36\20+(\245s\213\332"
  "\270\60c\242\324\262t\351\212\32-j\322\302\13M\332\364\301\17\10\63$\20+(\245s\213\332\64)"
  "U\202X\11r$\310\221-Z\360\244Q\264\326\246\313lU\21'\215\326\235\2\64&\20+(\305\262"
  "EM^\210\216\4\271!\304\210\20\33C\212\14!\212F\221!Ef\30\231\7\77 I\230n\0\65"
  "\36\17+(\25\66\274\31K\27\245\330\64iB\310h\265d\221%ST\244\11#u\246\0\66%\17"
  "+(-s\212\330\264(T\202\30ZJ\12\221@\342\302\205\231U\351p\227\252D\231\42mV%,"
  "\5\0\67\32\20+(\205\77 K\266,\265e\311\226%[\226l\265dkZ\266\206\0\70*\21#"
  "(\255\203\253\32\225\251\252\10\71\42\344\210\220#R\252LU\313V\265)U\242\134J,\323\225\60c"
  "\304Q\253\205\247\0\71%\17+(\255\202\251\326\64)S\242T:\334\245Zc\302\23\24\244\210\220%"
  "\213\214D\241\42lX%,\5\0:\14%:\270\204\7\352\241z\240\0;\21\345\272\266\204\7\352\241"
  "z\240F\222\21\223\4\2<\30\20*\70u\261%\217)Z\302F\331\311\223\352\26\256Sy\266\270\0"
  "=\13\217\261\70\205\277\207\375\203\37>\30\20*\70\5\341\204O\252[\307N\345ej\230,Rv\222"
  "\260p\0\77 \60\63H\245s\213\332\270\60c\242\324\262t\351\212\232\64y\360\244\311\262d\253\207\272"
  "l\235\1@`\335\353\26\346\363\200\232\77\10l\354d\331r\304\211\221\7Aj<\30\352J\214\231l"
  "\315\220aD\14\215\30E\210\324!b\243L\215\33e\210\334(C\343F\35\32\67j\304\240q\243F"
  "\14\42Eh\10\31Be\310\14z\60\210\320\221c\243\12\21$\17\226<\330\362 \221\221~\20\374="
  "\10\245\0A\63\66#xE\363 \314\3\307Z\261\266E\212\26)Z\244d!\222\205\12\26*W\214"
  "\134\261b\17\202=\10\365`\320\203A\5\313\24-R\264\204\321\305\5B\62\63;\210\5W\17\302<"
  "\20\362@H\71\23\5K\24,Q\260D\301\22\345\212<\10\363 \314\3!\17F\24,QR\37>"
  "H\361`\304\3!\217\0C\66\65+\210=\264\14\235=\10tM\301\42%M\24-QT=\210\362"
  " \312\203(\17\242<\210\362 \312\3)Z\242h\11\203F\12\226\71t\350A\60\207l\221\1D-"
  "\65\63\210\205\206\17B=\20\364`L\271#%\213\224\64Q\264D\321\265\372\323\245%J\232(Y\244"
  "\334\221\7c\36\10z\20\252!\0E!\62\63h\205\7\42\36\210x \342\201\210\322\365\372A\220\7"
  "A\36\4y\20\244t}\375\340\7\14F\21\60\63H\205\77H[o_\374\242l\375\267\0G\63\66"
  "+\230E\304\14\237=\20u\350\214\301\42FK\224]\17\242<\220\362@\312\3)\367\7aK\224-"
  "a\264\204I#\247\220<(\363b\24\223a\211\6H\20\63\63x\205\222\372/\37\374\301J\375/\13"
  "I\11%\63\230\204\377\203\2J\23/+(\325\372\377T\225^\255yP\244\315\262C\0K\67\64\63"
  "\210\205\202&\12\232(g\244\230\231R\206\12\231*c\254L\271\42\346J\30\134\271\222!\303vF\214"
  "\25\62U\310T)C\305\314\224+S\316HA\23%W\32L\16\60\63H\205\262\365\377\177\373\340\7"
  "\4MG\67\63\270\205\223.\337=\10\367 \334\203p\17D=\30\345b\324\10\25\243F\250 CB"
  "\5\31\22*\310\220P\62f\210\22\22D\224\220 \242\204\4\21%$\210\250\31\61F\315\31\65g\324"
  "\234QTHQ!E\25N'\63\63\210\205\222\13\27\262c\327\254\231\253G*\10\251(\243\204\214\222"
  "\42j\210\250)\241\210\204*\317\272\343p\313\2O-\67+\230\275\304-\337=\20\205\6\221\71\63&"
  "\215\224-R\266Di\375\272D\331\42e\213\230\64c\316\20\32T\17\304\275l\234\16\0P \62\63"
  "h\205Vo\36\4y \242\230\211r\13\365n\231\211\7\42\36\4q\304\254t\375\65\0Q\63W\353"
  "\247\275\304-\337=\20\205\6\221\71\63&\215\224-av\265~\31f\335\220\22\305J\20\61\245\306\24"
  "\42D\306\36\14{\60\354\301\270$\344A\11\1R\63\63\63\210\205G\17\204<\30\361`D\301\225z"
  "X\242\234\211\7B\36\204y\20\346\201\220r&\12\226(X\242`\211\202%\12\226(X\242`\211\202"
  "+\13S+\63+h\265\243\353Z=*d\246\134\21\202EH\22)~Ze;\207l\221\33W\251"
  "\262D\301\22\246\314<\10\344\214\345\61\0T\16\63#H\205\177\20\256x\375\377\337\1U\25\63\63\210"
  "\205\222\372\377_\256\63q\346\310\3\61\257\32\246\2V\63\64+h\205\242JK\24,R\260H\301\62"
  "\4\11\25+T\254\24\61b\205\212\25*G\210 !\202\204H\22!J\204(\21\242j\21cm\334"
  "z\202\0WR=+\370\205b\306\226YA\314X\21b\306\212\20\63V\244\20\242\62\205\20\225)\204"
  "\250\20!D\205\10\15\31D\252\10\65\244\212PR\214\10%\305\210\320\216\310\240!\344\210\14\32B\20"
  "\21JD(\21\241<v\324\230Ycf\215\31&H\232 \71\0X\63\64+h\5\203E\214\31\61"
  "V\310\252B\305\214\24,R\260DQ\245\211\21\37\67}\32mZ\225%\314\31)W\306\224\241R\245"
  "\314\30+b\316\204A\3Y\42\64+h\5\203\14\213\30\63S\254\220\215\212\25*f\244`\221\202K"
  "\225&F|\372x\371\372\207\0Z\33\61+H\205\77@\134\326\250\265e\215Zk\324\332\262F\255\65"
  "j\366\301\17\20[\16\350\263\266\204\7\215\350\377\377\217\36$\134\37)#\230\4q\342\306I\67l\234"
  "\270a\343\304\15\33'\335\260q\342\206\215\23\67l\234\4]\16\350\243\266\204\7\211\350\377\377\217\36\64"
  "^\31\316q:-\222\6\255C\206\212\10!\42\204\246!D\204\20\221a\230\21_\10R\240&\205\17"
  "\14`\13\245\60\275\204!D\206\20\31a\37O*(%TL\232\24R\226\214\340\241%-\316$K"
  "\206L\321\203\7&R\20\61C\0b!\60\63H\5\302\364\23C$\222\70ya\250D\261t\11\361"
  ".\235\252\22/\234\220@C\246\20\0c\34O*(-s\212\330\60)T\202T:\264t\227\214D"
  "\241\22mV\251\63\4\0d \60+He\372\221\21\62(\210\270xQhYB|\230\254D!\23"
  "OR\220AA\250\14\1e\36P*(-\203\252\330\70)T\242X\272\7\37\60&\134\214H\241\42"
  "\215X\245,\6\0f\24*#\270\254Bf\316\224\42F\315\203\67\304\350\377\33\0g)\60kF%"
  "#dP\20q\361\242\320\62e\11q\230LY\211B&\236\270AA\310\10\341d\312J\30\61\342\206"
  "\31\42\0h\26/\63H\5\262\364\33B$\220\270paHY\62t\370\337\21i\14$\63\230\204\7"
  "\1\37\374A\0j\20\6d\226\24:'B\377\377'\17L\224\0k(/\63\70\5\262\364\263T%"
  "\10\25!S\206H!\22\245\222\251R\265hQ\221\62d\312\20*B\212\10\251\22\304\222\25l\11$"
  "\63\230\204\377@\1m\60X\62\350\5\62\244\312\220@\221\342\1\212\7o\316\250*\205\214\30\62b\310"
  "\210!#\206\214\30\62b\310\210!#\206\214\30\62b\310\210!\243\0n\23O\62H\5\62eH\244"
  "p\361\300\220\62t\370\277#o\35Q*H\255\203\253\332<)U\242\234\272\224\270L\247\256D\251\42"
  "oZ-<\5\0p#\60sF\5\62\205H\240q\362\302P\211b\312\22\342\60\231\262E%^\70"
  "!\201\206L!\302\364\61\0q\37\60kF\255\42dP\20q\361\242\320\262\204\370\60Y\211B&\236"
  "\70IA\310\10a\372\1r\23J\62\330\4BFP<x\20\250T)b\364\317\0s P*\70"
  "\255s\213\232\24*B\254\4\261\22e\23\256[\247t]:\22\245J\270a\206\10\0t\22\311*\270"
  "\24R\264y\300\204\24\375WG\316\30*u\23O\62H\5r\370\277K\245\350\201\11\27)\310\224!"
  "v(Q\42\70\205r\352J\224*R\252\14)B\244\10\225)E\206\30\31bd\310\221 H\202 "
  "\11\222G\257-\134\14\0w\77Y\42\250\205R\245TUA\252\24\221B\225\224\71S\206\314\31Bd"
  "\316\20\42\63b\14!\42$\210\220\42\61f\4\61\22cF\20#\61f\4\61\64\350L\31\64e\320"
  "\224IrD\311\221\2x#P*\70\205b\213L\24*S\244P\21b\352P\232%k\362$:U"
  "EH\25)S\250\204\241e\5y'\60k\66\205b\232\21!U\244P\221B\64*R\250\10\61\42"
  "\304\210\20K\210\341QK\313VZ\362\244Q\243D\1z\21N*\30\205\37,\254\320\234\15\353\362\301"
  "\7\13{\31\11t\326\244\62F\216\24\42E\377\250\20\241q\244J\321\177u\306P\1|\11\343\273\226"
  "\204\377`\0}\32\11t\326\204Bf\16\225\242\377\252\24\271A\204\12\221\242\177r\304L!\0~\17"
  "\316p\71\225\241\246\226\30Yet\10\0\0\0\0\4\377\377\0";
const uint8_t u8g2_font_HelvetiPixel_tr[1028] U8G2_FONT_SECTION("u8g2_font_HelvetiPixel_tr") = 
  "_\0\3\2\4\4\2\4\5\11\14\0\375\10\376\10\376\1S\2\273\3\347 \5\0b\5!\6\201\343"
  "\304%\42\7\63\66Eb\11#\20\206\342U\222%\311\260\324\222hXjI\4$\16\225\242U\266T"
  "\224lK\224\312\26\1%\22\210\243N\26%Q\245\30\305Q\230\224*Q\226\0&\17\207\42\226\30e"
  "Q(&\221\246d\322\24'\6\61\266\304\0(\13\243\32U\22%Q[\224\5)\14\243\32E\26e"
  "QK\224D\0*\10\65\262ERY\32+\12U\246U\30\15R\30\1,\7\62\332L\242\0-\6"
  "\23*\305\0.\6\21\343D\0/\12\223\36UKT\211J\0\60\12\205\242\315\222\371\226,\0\61\10"
  "\203cU\262D=\62\12\205\242\315\222\205Y\307A\63\14\205\242\315\222\205\221\32j\311\2\64\16\206\342"
  "e\250%Q%K\206\61M\0\65\14\205\242\305\61\34\322PK\26\0\66\14\205\242\315\222\211C\222\331"
  "\222\5\67\14\205\242\305 fa\26fa\6\70\14\205\242\315\222i\311\222\331\222\5\71\14\205\242\315\222"
  "\331\222!\254,\0:\6Q\343D\26;\10r\332L\236(\0<\7V\346\245d\66=\10\65\252\305"
  "\240\16\2>\10V\346\205l\62\2\77\13\205\242\315\222i\215\71\24\1@\24\210\237\326\220\205I\264("
  "Q\242D\211\64Di\64(\0A\16\207\42^\234&a\222\225\6%U\3B\15\205\343\305\220d\332"
  "\240d\266A\1C\15\207\42\326\20\205r{\30\15\11\0D\15\206#\306\20eI\350-\31\42\0E"
  "\13\205\343\305\61\34\222\260\70\10F\12\205\343\305\61\34\222\260\21G\17\207\42\326\20\205rmH\223\60"
  "\32\22\0H\12\206#Fh\34\6\321\61I\6\201\343\304AJ\11\204b]oR\242\0K\17\206#"
  "F\226D\225LL\262\250\226\204\1L\10\205\343E\330\307AM\20\207cF\272\15\331RQ*R$"
  "Ej\0N\14\205\343E\66MJ\242$\322-O\14\207\42\326VI]\223,\233\0P\14\205\343\305"
  "\220d\266A\11\213\0Q\14\207\42\326VI]\223H\33\4R\15\206#\306\240\204\306a\211jI\30"
  "S\14\205\343\315\222\251j\252%\13\0T\11\205\242\305 \205}\2U\12\206#F\350\307dH\0V"
  "\17\207\42F\252&Y\224U\302$\215\63\0W\21\211\242F\226i\231V\351\224\64%m\305,\2X"
  "\16\205\343E\226\224\222,\314\222(\251\5Y\14\205\242E\246%\245$\13\233\0Z\11\206\342\305\65\354"
  "u\30[\10\242\33\305\322/\2\134\12\223\36ET\213jQ\1]\10\242\332\204\322/\3^\12U\256"
  "U\226DIM\13_\6\26\332\305\1`\6\42\372D\24a\13U\242\315\222%\203\226\14\1b\14\205"
  "\343EX\34\222\314\66(\0c\12U\242\315\222\211Y\262\0d\12\205\242e\313\240\331\222!e\12U"
  "\242\315\222\15C:\4f\12\203\42U\22%K\324\2g\14u\232\315\240\331\222!L\26\0h\12\205"
  "\343EX\34\222\314-i\7q\343D\62\10j\11\222\332L\226tQ\0k\13\204\243E\326\244$R"
  "R\12l\6\201\343\304Am\14Wc\306\242D\221\24I\221Tn\11U\343\305\220dn\1o\11U"
  "\242\315\222\331\222\5p\14u\333\305\220d\266A\11C\0q\12u\232\315\240\331\222!,r\11T\243"
  "E\62DY\15s\11Tb\315\20\212C\2t\11s\42M\224,Q-u\11U\343E\346\226\14\1"
  "v\13U\242E\246%\245$\213\0w\15W\42F\24I\221\322-\312\22\0x\12U\242E\226\324*"
  "\265\0y\14u\232E\246%\245$\13#\15z\11U\242\305\240\265\15\2{\13\263\26U\22\265dQ"
  "[\0|\7\241\333\304C\0}\13\263\26E\26\265%QK\4~\10$\257M\242$\0\0\0\0\4"
  "\377\377\0";

// helvB24 character widths (ASCII 32-126)
static const uint8_t width_helvB24[96] = {
     8,  7, 11, 21, 19, 30, 25,  8, 11, 11, 17, 20,  8, 12,  8, 18,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19,  8,  8, 20, 20, 20, 19,
    33, 25, 25, 25, 25, 23, 21, 28, 26, 10, 20, 24, 20, 30, 26, 28,
    24, 28, 24, 25, 23, 26, 24, 35, 24, 23, 21,  9, 18,  9, 19, 19,
    11, 20, 21, 18, 21, 19, 13, 21, 20,  8, 10, 19,  8, 30, 20, 21,
    21, 21, 13, 19, 12, 20, 18, 26, 18, 18, 16, 12,  8, 12, 22, 15
};

// ============================================================
// U8g2 font format decoder (for small font rendering)
// ============================================================
struct FontInfo {
    uint8_t glyph_cnt;
    uint8_t bbx_mode;
    uint8_t bits_per_0, bits_per_1;
    uint8_t bits_per_char_width, bits_per_char_height;
    uint8_t bits_per_char_x, bits_per_char_y, bits_per_delta_x;
    uint8_t max_char_width, max_char_height;
    int8_t x_offset, y_offset;
    int8_t ascent_A, descent_g;
    uint16_t start_pos_upper_A, start_pos_lower_a;
};

struct BitReader {
    const uint8_t* ptr;
    uint8_t bit_pos;
    
    BitReader(const uint8_t* p) : ptr(p), bit_pos(0) {}
    
    uint8_t read_bits(uint8_t cnt) {
        if (cnt == 0) return 0;
        uint8_t val;
        uint8_t bp = bit_pos;
        val = *ptr >> bp;
        bp += cnt;
        if (bp >= 8) {
            ++ptr;
            val |= *ptr << (8 - (bp - cnt));
            bp -= 8;
        }
        val &= (1U << cnt) - 1;
        bit_pos = bp;
        return val;
    }
    
    int8_t read_signed_bits(uint8_t cnt) {
        if (cnt == 0) return 0;
        uint8_t t = read_bits(cnt);
        uint8_t s = 8 - cnt;
        return (int8_t)((int8_t)(t << s) >> s);
    }
};

static void font_read_info(FontInfo& fi, const uint8_t* font) {
    fi.glyph_cnt            = font[0];
    fi.bbx_mode             = font[1];
    fi.bits_per_0           = font[2];
    fi.bits_per_1           = font[3];
    fi.bits_per_char_width  = font[4];
    fi.bits_per_char_height = font[5];
    fi.bits_per_char_x      = font[6];
    fi.bits_per_char_y      = font[7];
    fi.bits_per_delta_x     = font[8];
    fi.max_char_width       = font[9];
    fi.max_char_height      = font[10];
    fi.x_offset             = (int8_t)font[11];
    fi.y_offset             = (int8_t)font[12];
    fi.ascent_A             = (int8_t)font[13];
    fi.descent_g            = (int8_t)font[14];
    fi.start_pos_upper_A    = font[17] | ((uint16_t)font[18] << 8);
    fi.start_pos_lower_a    = font[19] | ((uint16_t)font[20] << 8);
}

static const uint8_t* find_glyph_data(const uint8_t* font, const FontInfo& fi, uint8_t encoding) {
    const uint8_t* data = font + 23;  // skip 23-byte header
    
    if (encoding >= 'a')
        data += fi.start_pos_lower_a;
    else if (encoding >= 'A')
        data += fi.start_pos_upper_A;
    
    while (true) {
        if (data[1] == 0) break;
        if (data[0] == encoding)
            return data + 2;  // skip encoding(1) + jump_offset(1) => glyph data
        data += data[1];
    }
    return nullptr;
}

// Decode a glyph: if render=true, draw pixels; returns delta_x (advance width)
static int decode_glyph(
    U8G2_LS013B7DH03_128X128_F_4W_SW_SPI* display,
    const uint8_t* glyph_data,
    const FontInfo& fi,
    int target_x, int target_y,
    bool render)
{
    BitReader br(glyph_data);
    
    int w = br.read_bits(fi.bits_per_char_width);
    int h = br.read_bits(fi.bits_per_char_height);
    int x = br.read_signed_bits(fi.bits_per_char_x);
    int y = br.read_signed_bits(fi.bits_per_char_y);
    int d = br.read_signed_bits(fi.bits_per_delta_x);
    
    if (!render || w == 0) return d;
    
    int pen_x = target_x + x;
    int pen_y = target_y - h - y;
    
    int gx = 0, gy = 0;
    
    while (gy < h) {
        uint8_t a = br.read_bits(fi.bits_per_0);
        uint8_t b = br.read_bits(fi.bits_per_1);
        
        do {
            // background pixels (skip)
            gx += a;
            while (gx >= w) { gx -= w; gy++; }
            
            // foreground pixels (draw)
            for (uint8_t i = 0; i < b; i++) {
                if (gy < h) {
                    int sx = pen_x + gx;
                    int sy = pen_y + gy;
                    if (display->getDrawColor() == 1) display->set_pixel(sx, sy);
                    else if (display->getDrawColor() == 0) display->clear_pixel(sx, sy);
                    else if (display->getDrawColor() == 2) display->xor_pixel(sx, sy);
                }
                gx++;
                while (gx >= w) { gx -= w; gy++; }
            }
        } while (br.read_bits(1) != 0);
    }
    
    return d;
}

// ============================================================
// Global objects
// ============================================================
U8G2_LS013B7DH03_128X128_F_4W_SW_SPI u8g2;
uint8_t* buf_ptr = nullptr;
uint16_t buf_len = 2048;
// Also defined in sim_config.h as: extern uint16_t buf_len;

// ============================================================
// SimU8g2 implementation
// ============================================================
U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::U8G2_LS013B7DH03_128X128_F_4W_SW_SPI()
    : drawColor(1)
    , glyph_w(8), glyph_h(8), glyph_baseline(7)
    , is_big_font(false)
    , font_direction(0)
    , cursor_x(0), cursor_y(0)
{
    clip = {0, 0, DISP_W - 1, DISP_H - 1};
    memset(buffer, 0, 2048);
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::set_pixel(int x, int y) {
    if (x < clip.x1 || x > clip.x2 || y < clip.y1 || y > clip.y2) return;
    if (x < 0 || x >= DISP_W || y < 0 || y >= DISP_H) return;
    buffer[y * 16 + (x >> 3)] |= (0x80 >> (x & 7));
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::clear_pixel(int x, int y) {
    if (x < clip.x1 || x > clip.x2 || y < clip.y1 || y > clip.y2) return;
    if (x < 0 || x >= DISP_W || y < 0 || y >= DISP_H) return;
    buffer[y * 16 + (x >> 3)] &= ~(0x80 >> (x & 7));
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::xor_pixel(int x, int y) {
    if (x < clip.x1 || x > clip.x2 || y < clip.y1 || y > clip.y2) return;
    if (x < 0 || x >= DISP_W || y < 0 || y >= DISP_H) return;
    buffer[y * 16 + (x >> 3)] ^= (0x80 >> (x & 7));
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::hline(int x, int y, int w) {
    for (int i = 0; i < w; ++i) {
        int px = x + i;
        if (px < clip.x1 || px > clip.x2) continue;
        if (y < clip.y1 || y > clip.y2) continue;
        if (px < 0 || px >= DISP_W || y < 0 || y >= DISP_H) continue;
        if (drawColor == 1) set_pixel(px, y);
        else if (drawColor == 0) clear_pixel(px, y);
        else if (drawColor == 2) xor_pixel(px, y);
    }
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::vline(int x, int y, int h) {
    for (int i = 0; i < h; ++i) {
        int py = y + i;
        if (x < clip.x1 || x > clip.x2) continue;
        if (py < clip.y1 || py > clip.y2) continue;
        if (x < 0 || x >= DISP_W || py < 0 || py >= DISP_H) continue;
        if (drawColor == 1) set_pixel(x, py);
        else if (drawColor == 0) clear_pixel(x, py);
        else if (drawColor == 2) xor_pixel(x, py);
    }
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::drawBox(int x, int y, int w, int h) {
    int x2 = x + w - 1, y2 = y + h - 1;
    int cx1 = (std::max)(clip.x1, x), cy1 = (std::max)(clip.y1, y);
    int cx2 = (std::min)(clip.x2, x2), cy2 = (std::min)(clip.y2, y2);
    for (int row = cy1; row <= cy2; ++row)
        for (int col = cx1; col <= cx2; ++col) {
            if (drawColor == 1) set_pixel(col, row);
            else if (drawColor == 0) clear_pixel(col, row);
            else if (drawColor == 2) xor_pixel(col, row);
        }
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::drawFrame(int x, int y, int w, int h) {
    hline(x, y, w);
    hline(x, y + h - 1, w);
    vline(x, y, h);
    vline(x + w - 1, y, h);
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::drawRBox(int x, int y, int w, int h, float r) {
    int rr = (int)(r + 0.5f);
    if (rr <= 0) { drawBox(x, y, w, h); return; }

    // 匹配U8g2原生drawRBox实现:
    // 1. 画4个1/4圆角（使用drawDisc的象限模式）
    // 2. 画顶部和底部横条（连接左右圆角之间）
    // 3. 画中间填充区域

    // 圆角圆心坐标
    int cx_l = x + rr;          // 左侧圆心x
    int cx_r = x + w - rr - 1;  // 右侧圆心x
    int cy_u = y + rr;          // 上方圆心y
    int cy_l = y + h - rr - 1;  // 下方圆心y

    // 画4个1/4圆盘（使用像素循环，匹配u8g2 drawDisc象限方式）
    // UL: px <= cx_l && py <= cy_u  (Upper-Left)
    // UR: px >= cx_r && py <= cy_u  (Upper-Right)
    // LL: px <= cx_l && py >= cy_l  (Lower-Left)
    // LR: px >= cx_r && py >= cy_l  (Lower-Right)
    for (int dy = 0; dy <= rr; ++dy) {
        for (int dx = 0; dx <= rr; ++dx) {
            if (dx * dx + dy * dy > rr * rr) continue; // 在圆内才绘制

            // 上左象限
            { int px = cx_l - dx, py = cy_u - dy; if (drawColor == 1) set_pixel(px, py); else if (drawColor == 0) clear_pixel(px, py); else if (drawColor == 2) xor_pixel(px, py); }
            // 上右象限
            { int px = cx_r + dx, py = cy_u - dy; if (drawColor == 1) set_pixel(px, py); else if (drawColor == 0) clear_pixel(px, py); else if (drawColor == 2) xor_pixel(px, py); }
            // 下左象限
            { int px = cx_l - dx, py = cy_l + dy; if (drawColor == 1) set_pixel(px, py); else if (drawColor == 0) clear_pixel(px, py); else if (drawColor == 2) xor_pixel(px, py); }
            // 下右象限
            { int px = cx_r + dx, py = cy_l + dy; if (drawColor == 1) set_pixel(px, py); else if (drawColor == 0) clear_pixel(px, py); else if (drawColor == 2) xor_pixel(px, py); }
        }
    }

    // 顶部横条（在左右圆角之间，不重叠）
    int inner_w = w - rr * 2;
    if (inner_w >= 3) {
        drawBox(cx_l + 1, y, inner_w - 2, rr + 1);
        // 底部横条
        drawBox(cx_l + 1, cy_l, inner_w - 2, rr + 1);
    }

    // 中间填充区域（全宽，在上下圆角之间，不重叠）
    int inner_h = h - rr * 2;
    if (inner_h >= 3) {
        drawBox(x, cy_u + 1, w, inner_h - 2);
    }
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::drawRFrame(int x, int y, int w, int h, float r) {
    int rr = (int)(r + 0.5f);
    if (rr <= 0) { drawFrame(x, y, w, h); return; }
    int saved = drawColor;
    drawColor = 1;
    drawRBox(x, y, w, h, r);
    drawColor = 0;
    drawBox(x + 1, y + 1, w - 2, h - 2);
    drawColor = (saved == 2) ? 1 : saved;
    if (saved == 2) {
        drawColor = 1;
        drawRBox(x, y, w, h, r);
        drawColor = 0;
        drawBox(x + 1, y + 1, w - 2, h - 2);
        drawColor = saved;
    }
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::drawHLine(int x, int y, int w) { hline(x, y, w); }
void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::drawVLine(int x, int y, int h) { vline(x, y, h); }

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::drawLine(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        if (drawColor == 1) set_pixel(x1, y1);
        else if (drawColor == 0) clear_pixel(x1, y1);
        else if (drawColor == 2) xor_pixel(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

int U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::getStrWidth(const char* s) {
    if (!s) return 0;
    if (is_big_font) {
        // Use GDI to measure text width for big font (always 36px)
        HDC hdc = CreateCompatibleDC(NULL);
        BITMAPINFO bmi = {0};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = 1;
        bmi.bmiHeader.biHeight = 1;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        uint32_t* px;
        HBITMAP bmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&px, NULL, 0);
        HGDIOBJ old_bmp = SelectObject(hdc, bmp);

        HFONT hFont = CreateFontA(-36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                  CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                  DEFAULT_PITCH | FF_DONTCARE, "Arial");
        HGDIOBJ old_font = SelectObject(hdc, hFont);

        SIZE sz;
        GetTextExtentPoint32A(hdc, s, (int)strlen(s), &sz);

        SelectObject(hdc, old_font);
        SelectObject(hdc, old_bmp);
        DeleteObject(hFont);
        DeleteObject(bmp);
        DeleteDC(hdc);
        return sz.cx;
    }
    // Use U8g2 font decoder for HelvetiPixel font
    FontInfo fi;
    font_read_info(fi, u8g2_font_HelvetiPixel_tr);
    int w = 0;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if (c >= 32 && c <= 126) {
            const uint8_t* gd = find_glyph_data(u8g2_font_HelvetiPixel_tr, fi, c);
            if (gd) {
                w += decode_glyph(nullptr, gd, fi, 0, 0, false);
            }
        }
        ++s;
    }
    return w;
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::drawStr(int x, int y, const char* s) {
    if (!s) return;

    if (is_big_font && font_direction == 0) {
        // Render big font using Windows GDI (always 36px regardless of screen DPI)
        HDC hdc = CreateCompatibleDC(NULL);
        // Use 48 rows to ensure enough room for the 36px font (ascent ~29, descent ~7)
        const int BITMAP_ROWS = 48;
        BITMAPINFO bmi = {0};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = DISP_W;
        bmi.bmiHeader.biHeight = -BITMAP_ROWS; // top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        uint32_t* pixels;
        HBITMAP bmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&pixels, NULL, 0);
        HGDIOBJ old_bmp = SelectObject(hdc, bmp);

        // Always use 36px font (negative = char height in device units)
        HFONT hFont = CreateFontA(-36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                  CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                  DEFAULT_PITCH | FF_DONTCARE, "Arial");
        HGDIOBJ old_font = SelectObject(hdc, hFont);

        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, OPAQUE);

        // Fill bitmap with black
        memset(pixels, 0, DISP_W * BITMAP_ROWS * 4);

        // Get font metrics to determine baseline position
        TEXTMETRICA tm;
        GetTextMetricsA(hdc, &tm);
        int ascent = tm.tmAscent;

        // Draw text with top at row 0 so the full character fits within the bitmap
        // (TA_TOP default: y specifies the top of the character cell)
        TextOutA(hdc, 0, 0, s, (int)strlen(s));

        // baseline_y maps bitmap row=ascent (the baseline) to target display y-position
        // The u8g2 drawStr y coordinate is the baseline
        int baseline_y = y - ascent;
        // Copy rendered pixels to display buffer
        for (int row = 0; row < BITMAP_ROWS; ++row) {
            int sy = baseline_y + row;
            if (sy < 0 || sy >= DISP_H) continue;
            for (int col = 0; col < DISP_W; ++col) {
                int sx = x + col;
                if (sx < 0 || sx >= DISP_W) continue;
                if (pixels[row * DISP_W + col] != 0) {
                    if (drawColor == 1) set_pixel(sx, sy);
                    else if (drawColor == 0) clear_pixel(sx, sy);
                    else if (drawColor == 2) xor_pixel(sx, sy);
                }
            }
        }

        SelectObject(hdc, old_font);
        SelectObject(hdc, old_bmp);
        DeleteObject(hFont);
        DeleteObject(bmp);
        DeleteDC(hdc);
        return;
    }

    // Use U8g2 font decoder for HelvetiPixel font
    FontInfo fi;
    font_read_info(fi, u8g2_font_HelvetiPixel_tr);
    int px = x;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if (c >= 32 && c <= 126) {
            const uint8_t* gd = find_glyph_data(u8g2_font_HelvetiPixel_tr, fi, c);
            if (gd) {
                int d = decode_glyph(this, gd, fi, px, y, true);
                px += d;
            }
        }
        ++s;
    }
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::drawXBMP(int x, int y, int w, int h, const uint8_t* bitmap) {
    int bpr = (w + 7) / 8;
    for (int row = 0; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
            int byte_idx = row * bpr + (col >> 3);
            // U8g2的drawXBMP使用LSB-first: bit0(0x01)=最左像素, bit7(0x80)=最右像素
            if (bitmap[byte_idx] & (1 << (col & 7))) {
                int sx = x + col, sy = y + row;
                if (sx < 0 || sx >= DISP_W || sy < 0 || sy >= DISP_H) continue;
                if (drawColor == 1) set_pixel(sx, sy);
                else if (drawColor == 0) clear_pixel(sx, sy);
                else if (drawColor == 2) xor_pixel(sx, sy);
            }
        }
    }
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::setDrawColor(int c) { drawColor = c; }
int U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::getDrawColor() { return drawColor; }

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::setFont(const uint8_t* f) {
    if (f == u8g2_font_helvB24_tr || f == (const uint8_t*)1) {
        is_big_font = true;
        glyph_w = 24; glyph_h = 24; glyph_baseline = 18;
    } else {
        is_big_font = false;
        glyph_w = 8; glyph_h = 8; glyph_baseline = 7;
    }
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::setFontDirection(int d) { font_direction = d; }
void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::setCursor(int x, int y) { cursor_x = x; cursor_y = y; }
void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::setContrast(int) {}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::clearBuffer() { memset(buffer, 0, 2048); }
void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::sendBuffer() {}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::print(const char* s) {
    if (!s) return;
    drawStr(cursor_x, cursor_y, s);
    cursor_x += getStrWidth(s);
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::print(int val) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", val);
    print(buf);
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::print(float val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f", (double)val);
    print(buf);
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::begin() {
    clearBuffer();
    buf_ptr = buffer;
    buf_len = 2048;
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::setClipWindow(int x1, int y1, int x2, int y2) {
    clip.x1 = (std::max)(0, x1);
    clip.y1 = (std::max)(0, y1);
    clip.x2 = (std::min)(DISP_W - 1, x2);
    clip.y2 = (std::min)(DISP_H - 1, y2);
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::setMaxClipWindow() {
    clip = {0, 0, DISP_W - 1, DISP_H - 1};
}

// ============================================================
// Compat stub structures for getU8x8/getU8g2
// ============================================================
struct u8x8_dummy { uint8_t x; };
struct u8g2_dummy { int tile_curr_row; };
static u8x8_dummy g_u8x8;
static u8g2_dummy g_u8g2;

u8x8_t* U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::getU8x8() {
    return reinterpret_cast<u8x8_t*>(&g_u8x8);
}

u8g2_t* U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::getU8g2() {
    return reinterpret_cast<u8g2_t*>(&g_u8g2);
}

// ============================================================
// Display init
// ============================================================
void lcd_init() {
    u8g2.begin();
    buf_ptr = u8g2.getBufferPtr();
    buf_len = 2048;
}

void lcd_reset_vcom() {}

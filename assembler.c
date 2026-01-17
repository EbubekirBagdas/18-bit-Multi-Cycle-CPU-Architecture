#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ---------------- ISA / OPCODE TANIMLARI ---------------- */

typedef enum {
    OP_ADD, OP_SUB, OP_NAND, OP_NOR, OP_SRL, OP_SRA,
    OP_ADDI, OP_SUBI, OP_NANDI, OP_NORI,
    OP_JUMP, OP_JAL,
    OP_LD, OP_ST,
    OP_LUI, OP_CMOV,
    OP_PUSH, OP_POP,
    OP_INVALID
} OpCodeType;

typedef struct {
    const char *mnemonic;
    OpCodeType type;
    int opcode_bits;   /* 5-bit (0..31) */
} OpInfo;

OpInfo op_table[] = {
    {"ADD",   OP_ADD,   0x00},
    {"SUB",   OP_SUB,   0x01},
    {"NAND",  OP_NAND,  0x02},
    {"NOR",   OP_NOR,   0x03},
    {"SRL",   OP_SRL,   0x04},
    {"SRA",   OP_SRA,   0x05},
    {"ADDI",  OP_ADDI,  0x06},
    {"SUBI",  OP_SUBI,  0x07},
    {"NANDI", OP_NANDI, 0x08},
    {"NORI",  OP_NORI,  0x09},
    {"JUMP",  OP_JUMP,  0x0A},
    {"JAL",   OP_JAL,   0x0B},
    {"LD",    OP_LD,    0x0C},
    {"ST",    OP_ST,    0x0D},
    {"LUI",   OP_LUI,   0x0E},
    {"CMOV",  OP_CMOV,  0x0F},
    {"PUSH",  OP_PUSH,  0x10},
    {"POP",   OP_POP,   0x11},
};

const int OP_TABLE_SIZE = sizeof(op_table) / sizeof(op_table[0]);

/* ---------------- YARDIMCI FONKSİYONLAR ---------------- */

static void strtoupper_inplace(char *s) {
    while (*s) {
        *s = (char)toupper((unsigned char)*s);
        s++;
    }
}

static OpInfo* find_op(const char *mnemonic) {
    int i;
    for (i = 0; i < OP_TABLE_SIZE; i++) {
        if (strcmp(op_table[i].mnemonic, mnemonic) == 0)
            return &op_table[i];
    }
    return NULL;
}

/* "R5" -> 5, "r10" -> 10 */
static int parse_reg(const char *token) {
    if (!token) return -1;
    if (token[0] != 'R' && token[0] != 'r') return -1;
    char *endptr;
    long val = strtol(token + 1, &endptr, 10);
    if (*endptr != '\0') return -1;
    if (val < 0 || val > 15) return -1;
    return (int)val;
}

/* sayısal immediate / offset / addr parse */
static int parse_imm(const char *token) {
    if (!token) return 0;
    char *endptr;
    long val = strtol(token, &endptr, 0);  /* 0 -> 10, 16, 8 otomatik */
    if (*endptr != '\0') {
        fprintf(stderr, "Warning: immediate '%s' parsed partially.\n", token);
    }
    return (int)val;
}

/* Satırın başındaki ve sonundaki whitespace'i sil (in place) */
static void trim(char *s) {
    char *end;
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) {
        /* boş string */
        s[0] = '\0';
        return;
    }
    end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
}

/* ---------------- ENCODE FONKSİYONLARI ---------------- */

static unsigned int encode_rtype(int opcode, int dst, int src1, int src2) {
    unsigned int inst = 0;
    inst |= ((unsigned int)(opcode & 0x1F)) << 13;
    inst |= ((unsigned int)(dst    & 0x0F)) << 9;
    inst |= ((unsigned int)(src1   & 0x0F)) << 5;
    inst |= ((unsigned int)(src2   & 0x0F)) << 1;
    /* bit0 = 0 */
    return inst;
}

static unsigned int encode_itype(int opcode, int dst, int src1, int imm) {
    unsigned int inst = 0;
    imm &= 0x1F; /* 5 bit */
    inst |= ((unsigned int)(opcode & 0x1F)) << 13;
    inst |= ((unsigned int)(dst    & 0x0F)) << 9;
    inst |= ((unsigned int)(src1   & 0x0F)) << 5;
    inst |= (unsigned int)imm;
    return inst;
}

static unsigned int encode_mem(int opcode, int reg, int addr) {
    unsigned int inst = 0;
    addr &= 0x1FF;  /* 9 bit */
    inst |= ((unsigned int)(opcode & 0x1F)) << 13;
    inst |= ((unsigned int)(reg    & 0x0F)) << 9;
    inst |= (unsigned int)addr;
    return inst;
}

static unsigned int encode_jump(int opcode, int offset) {
    unsigned int inst = 0;
    offset &= 0x1FFF; /* 13 bit signed */
    inst |= ((unsigned int)(opcode & 0x1F)) << 13;
    inst |= (unsigned int)offset;
    return inst;
}

static unsigned int encode_lui(int opcode, int dst, int imm) {
    unsigned int inst = 0;
    imm &= 0x1FF; /* 9 bit */
    inst |= ((unsigned int)(opcode & 0x1F)) << 13;
    inst |= ((unsigned int)(dst    & 0x0F)) << 9;
    inst |= (unsigned int)imm;
    return inst;
}

/* ---------------- TEK SATIR ASSEMBLE ---------------- */

static void assemble_line(const char *line_in, FILE *out, int *word_count) {
    char buf[256];
    char *tok;
    OpInfo *op;
    unsigned int inst;

    /* satırı local buffer'a kopyala, üzerinde oynayacağız */
    strncpy(buf, line_in, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    /* yorumları sil: //, ; veya # sonrası */
    char *cmt = strstr(buf, "//");
    if (cmt) *cmt = '\0';
    cmt = strchr(buf, ';');
    if (cmt) *cmt = '\0';
    cmt = strchr(buf, '#');
    if (cmt) *cmt = '\0';

    trim(buf);
    if (buf[0] == '\0') return; /* boş satır */

    /* ilk token: mnemonic veya label olabilir (label: "loop:") */
    tok = strtok(buf, " \t\r\n,");
    if (!tok) return;

    /* label ise (son karakter ':'), şimdilik ignore (label desteği yok) */
    {
        size_t len = strlen(tok);
        if (len > 0 && tok[len - 1] == ':') {
            fprintf(stderr, "Warning: label '%s' ignored (label support not implemented).\n", tok);
            return;
        }
    }

    /* mnemonic'i upper'a çevir */
    strtoupper_inplace(tok);

    op = find_op(tok);
    if (!op) {
        fprintf(stderr, "Error: unknown opcode '%s'\n", tok);
        return;
    }

    inst = 0;

    /* R-Type (ADD, SUB, NAND, NOR, SRL, SRA) */
    if (op->type == OP_ADD || op->type == OP_SUB || op->type == OP_NAND ||
        op->type == OP_NOR || op->type == OP_SRL || op->type == OP_SRA) {

        char *dst_s  = strtok(NULL, " \t\r\n,");
        char *src1_s = strtok(NULL, " \t\r\n,");
        char *src2_s = strtok(NULL, " \t\r\n,");
        int dst, src1, src2;

        if (!dst_s || !src1_s || !src2_s) {
            fprintf(stderr, "Error: not enough operands for %s\n", op->mnemonic);
            return;
        }

        dst  = parse_reg(dst_s);
        src1 = parse_reg(src1_s);
        src2 = parse_reg(src2_s);

        if (dst < 0 || src1 < 0 || src2 < 0) {
            fprintf(stderr, "Error: invalid register in %s\n", op->mnemonic);
            return;
        }

        inst = encode_rtype(op->opcode_bits, dst, src1, src2);
    }
    /* CMOV REG1 REG2 REG3 (REG1==0 ise REG2=REG3) */
    else if (op->type == OP_CMOV) {
        char *reg1_s = strtok(NULL, " \t\r\n,");
        char *reg2_s = strtok(NULL, " \t\r\n,");
        char *reg3_s = strtok(NULL, " \t\r\n,");
        int reg1, reg2, reg3;

        if (!reg1_s || !reg2_s || !reg3_s) {
            fprintf(stderr, "Error: not enough operands for CMOV\n");
            return;
        }

        reg1 = parse_reg(reg1_s); /* şart kontrol edilen register */
        reg2 = parse_reg(reg2_s); /* hedef register */
        reg3 = parse_reg(reg3_s); /* source değer register'ı */

        if (reg1 < 0 || reg2 < 0 || reg3 < 0) {
            fprintf(stderr, "Error: invalid register in CMOV\n");
            return;
        }

        /* encode_rtype(opcode, dst, src1, src2) */
        inst = encode_rtype(op->opcode_bits, reg2, reg1, reg3);
    }
    /* I-Type (ADDI, SUBI, NANDI, NORI) */
    else if (op->type == OP_ADDI || op->type == OP_SUBI ||
             op->type == OP_NANDI || op->type == OP_NORI) {

        char *dst_s  = strtok(NULL, " \t\r\n,");
        char *src1_s = strtok(NULL, " \t\r\n,");
        char *imm_s  = strtok(NULL, " \t\r\n,");
        int dst, src1, imm;

        if (!dst_s || !src1_s || !imm_s) {
            fprintf(stderr, "Error: not enough operands for %s\n", op->mnemonic);
            return;
        }

        dst  = parse_reg(dst_s);
        src1 = parse_reg(src1_s);
        imm  = parse_imm(imm_s);

        if (dst < 0 || src1 < 0) {
            fprintf(stderr, "Error: invalid register in %s\n", op->mnemonic);
            return;
        }

        inst = encode_itype(op->opcode_bits, dst, src1, imm);
    }
    /* Memory-Type (LD, ST): LD R5, 12 / ST R3, 40 */
    else if (op->type == OP_LD || op->type == OP_ST) {
        char *reg_s  = strtok(NULL, " \t\r\n,");
        char *addr_s = strtok(NULL, " \t\r\n,");
        int reg, addr;

        if (!reg_s || !addr_s) {
            fprintf(stderr, "Error: not enough operands for %s\n", op->mnemonic);
            return;
        }

        reg  = parse_reg(reg_s);
        addr = parse_imm(addr_s);

        if (reg < 0) {
            fprintf(stderr, "Error: invalid register in %s\n", op->mnemonic);
            return;
        }

        inst = encode_mem(op->opcode_bits, reg, addr);
    }
    /* JUMP / JAL: JUMP -3, JAL 4 */
    else if (op->type == OP_JUMP || op->type == OP_JAL) {
        char *off_s = strtok(NULL, " \t\r\n,");
        int offset;

        if (!off_s) {
            fprintf(stderr, "Error: not enough operands for %s\n", op->mnemonic);
            return;
        }

        offset = parse_imm(off_s);
        inst = encode_jump(op->opcode_bits, offset);
    }
    /* LUI DST, IMM */
    else if (op->type == OP_LUI) {
        char *dst_s = strtok(NULL, " \t\r\n,");
        char *imm_s = strtok(NULL, " \t\r\n,");
        int dst, imm;

        if (!dst_s || !imm_s) {
            fprintf(stderr, "Error: not enough operands for LUI\n");
            return;
        }

        dst = parse_reg(dst_s);
        imm = parse_imm(imm_s);

        if (dst < 0) {
            fprintf(stderr, "Error: invalid register in LUI\n");
            return;
        }

        inst = encode_lui(op->opcode_bits, dst, imm);
    }
    /* PUSH / POP SRC */
    else if (op->type == OP_PUSH || op->type == OP_POP) {
        char *src_s = strtok(NULL, " \t\r\n,");
        int src;

        if (!src_s) {
            fprintf(stderr, "Error: not enough operands for %s\n", op->mnemonic);
            return;
        }

        src = parse_reg(src_s);
        if (src < 0) {
            fprintf(stderr, "Error: invalid register in %s\n", op->mnemonic);
            return;
        }

        /* PUSH/POP format: opcode + src + alt 9 bit 0 */
        inst = encode_mem(op->opcode_bits, src, 0);
    }
    else {
        fprintf(stderr, "Error: unhandled opcode type (internal)\n");
        return;
    }

    /* 18-bit mask ve 5 haneli hex (logisim) */
    /* BURASI DEĞİŞTİ: %05x -> %05X */
    inst &= 0x3FFFF;
    fprintf(out, "%05X ", inst);
    (*word_count)++;

    /* okunabilirlik için örneğin her 8 kelimede bir newline atabiliriz */
    if ((*word_count) % 8 == 0) {
        fprintf(out, "\n");
    }
}

/* ---------------- MAIN ---------------- */

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Kullanım: %s input.asm output.hex\n", argv[0]);
        return 1;
    }

    const char *input_name  = argv[1];
    const char *output_name = argv[2];

    FILE *fin  = fopen(input_name, "r");
    if (!fin) {
        perror("Input dosyası açılamadı");
        return 1;
    }

    FILE *fout = fopen(output_name, "w");
    if (!fout) {
        perror("Output dosyası açılamadı");
        fclose(fin);
        return 1;
    }

    /* Logisim v2.0 raw header */
    fprintf(fout, "v2.0 raw\n");

    char line[256];
    int word_count = 0;

    while (fgets(line, sizeof(line), fin)) {
        assemble_line(line, fout, &word_count);
    }

    /* son satırı kapatmak için newline */
    if (word_count % 8 != 0) {
        fprintf(fout, "\n");
    }

    fclose(fin);
    fclose(fout);

    printf("Assembly bitti. %d kelime output.hex dosyasına yazıldı.\n", word_count);
    return 0;
}
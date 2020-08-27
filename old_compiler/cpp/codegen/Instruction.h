/*!
 * @file: Instruction.h
 * x86-64 Instruction encoding.
 */

#ifndef JAK_INSTRUCTION_H
#define JAK_INSTRUCTION_H

#include <cstdint>

/*!
 * The ModRM byte
 */
struct ModRM {
  uint8_t mod;
  uint8_t reg_op;
  uint8_t rm;

  uint8_t operator()() { return (mod << 6) | (reg_op << 3) | (rm << 0); }
};

/*!
 * The SIB Byte
 */
struct SIB {
  uint8_t scale, index, base;

  uint8_t operator()() { return (scale << 6) | (index << 3) | (base << 0); }
};

/*!
 * An Immediate (either imm or disp)
 */
struct Imm {
  Imm() = default;
  Imm(uint8_t sz, uint64_t v) : size(sz), value(v) {}
  uint8_t size;
  union {
    uint64_t value;
    uint8_t v_arr[8];
  };
};

/*!
 * The REX prefix byte
 */
struct REX {
  REX(bool w = false, bool r = false, bool x = false, bool b = false) : W(w), R(r), X(x), B(b) {}
  // W - 64-bit operands
  // R - reg extension
  // X - SIB i extnsion
  // B - other extension
  bool W, R, X, B;
  uint8_t operator()() { return (1 << 6) | (W << 3) | (R << 2) | (X << 1) | (B << 0); }
};

/*!
 * A high-level description of an x86-64 opcode.  It can emit itself.
 */
struct Instruction {
  Instruction(uint8_t opcode) : op(opcode) {}
  uint8_t op;

  bool op2_set = false;
  uint8_t op2;

  bool op3_set = false;
  uint8_t op3;

  // if true, don't emit anything
  bool is_null = false;

  // flag to indicate it's the first instruction of a function and needs align and type tag
  bool is_function_start = false;

  // the rex byte
  bool set_rex = false;
  uint8_t m_rex = 0;

  // the modrm byte
  bool set_modrm = false;
  uint8_t m_modrm = 0;

  // the sib byte
  bool set_sib = false;
  uint8_t m_sib = 0;

  // the displacement
  bool set_disp_imm = false;
  Imm disp;

  // the immediate
  bool set_imm = false;
  Imm imm;

  // which IR instruction does this go with?
  // this is only set for the first instruction generated from an IR.
  int ir_index = -1;

  /*!
   * Move opcode byte 0 to before the rex prefix.
   */
  void swap_op0_rex() {
    if (!set_rex)
      return;
    auto temp = op;
    op = m_rex;
    m_rex = temp;
  }

  void set(REX r) {
    m_rex = r();
    set_rex = true;
  }

  void set(ModRM modrm) {
    m_modrm = modrm();
    set_modrm = true;
  }

  void set(SIB sib) {
    m_sib = sib();
    set_sib = true;
  }

  void set_disp(Imm i) {
    disp = i;
    set_disp_imm = true;
  }

  void set(Imm i) {
    imm = i;
    set_imm = true;
  }

  void set_op2(uint8_t b) {
    op2_set = true;
    op2 = b;
  }

  void set_op3(uint8_t b) {
    op3_set = true;
    op3 = b;
  }

  /*!
   * Set modrm and rex as needed for two regs.
   */
  void set_modrm_and_rex(uint8_t reg, uint8_t rm, uint8_t mod, bool rex_w = false) {
    bool rex_b = false, rex_r = false;

    if (rm >= 8) {
      rm -= 8;
      rex_b = true;
    }

    if (reg >= 8) {
      reg -= 8;
      rex_r = true;
    }

    ModRM modrm;
    modrm.mod = mod;
    modrm.reg_op = reg;
    modrm.rm = rm;

    set(modrm);

    if (rex_b || rex_w || rex_r) {
      set(REX(rex_w, rex_r, false, rex_b));
    }
  }

  /*!
   * Set modrm and rex as needed for two regs for an addressing mode.
   * Will set SIB if R12 or RSP indexing is used.
   */
  void set_modrm_and_rex_for_addr(uint8_t reg, uint8_t rm, uint8_t mod, bool rex_w = false) {
    bool rex_b = false, rex_r = false;

    if (rm >= 8) {
      rm -= 8;
      rex_b = true;
    }

    if (reg >= 8) {
      reg -= 8;
      rex_r = true;
    }

    ModRM modrm;
    modrm.mod = mod;
    modrm.reg_op = reg;
    modrm.rm = rm;

    set(modrm);

    if (rm == 4) {
      SIB sib;
      sib.scale = 0;
      sib.base = 4;
      sib.index = 4;

      set(sib);
    }

    if (rex_b || rex_w || rex_r) {
      set(REX(rex_w, rex_r, false, rex_b));
    }
  }

  void add_rex() {
    if (!set_rex) {
      set(REX());
    }
  }

  /*!
   * Set up modrm and rex for the commonly used 32-bit immediate displacement indexing mode.
   */
  void set_modrm_rex_sib_for_reg_reg_disp32(uint8_t reg, uint8_t mod, uint8_t rm, bool rex_w) {
    ModRM modrm;

    bool rex_r = false;
    if (reg >= 8) {
      reg -= 8;
      rex_r = true;
    }
    modrm.reg_op = reg;

    modrm.mod = mod;

    modrm.rm = 4;  // use sib

    SIB sib;
    sib.scale = 0;
    sib.index = 4;
    bool rex_b = false;
    if (rm >= 8) {
      rex_b = true;
      rm -= 8;
    }

    sib.base = rm;

    set(modrm);
    set(sib);

    if (rex_r || rex_w || rex_b) {
      set(REX(rex_w, rex_r, false, rex_b));
    }
  }

  /*!
   * Get the position of the disp immediate relative to the start of the instruction
   */
  int offset_of_disp() {
    if (is_null)
      return 0;
    assert(set_disp_imm);
    int offset = 0;
    if (set_rex)
      offset++;
    offset++;  // opcode
    if (op2_set)
      offset++;
    if (op3_set)
      offset++;
    if (set_modrm)
      offset++;
    if (set_sib)
      offset++;
    return offset;
  }

  /*!
   * Get the position of the imm immediate relative to the start of the instruction
   */
  int offset_of_imm() {
    if (is_null)
      return 0;
    assert(set_imm);
    int offset = 0;
    if (set_rex)
      offset++;
    offset++;  // opcode
    if (op2_set)
      offset++;
    if (op3_set)
      offset++;
    if (set_modrm)
      offset++;
    if (set_sib)
      offset++;
    if (set_disp_imm)
      offset += disp.size;
    return offset;
  }

  /*!
   * Emit into a buffer and return how many bytes written (can be zero)
   */
  uint8_t emit(uint8_t* buffer) {
    if (is_null)
      return 0;
    uint8_t count = 0;
    if (set_rex) {
      buffer[count++] = m_rex;
    }

    buffer[count++] = op;

    if (op2_set) {
      buffer[count++] = op2;
    }

    if (op3_set) {
      buffer[count++] = op3;
    }

    if (set_modrm) {
      buffer[count++] = m_modrm;
    }

    if (set_sib) {
      buffer[count++] = m_sib;
    }

    if (set_disp_imm) {
      for (int i = 0; i < disp.size; i++) {
        buffer[count++] = disp.v_arr[i];
      }
    }

    if (set_imm) {
      for (int i = 0; i < imm.size; i++) {
        buffer[count++] = imm.v_arr[i];
      }
    }
    return count;
  }
};

#endif  // JAK_INSTRUCTION_H
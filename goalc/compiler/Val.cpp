#include "third-party/fmt/core.h"
#include "Val.h"
#include "Env.h"
#include "IR.h"

/*!
 * Fallback to_gpr if a more optimized one is not provided.
 */
RegVal* Val::to_gpr(Env* fe) {
  // TODO - handle 128-bit stuff here!
  auto rv = to_reg(fe);
  if (rv->ireg().kind == emitter::RegKind::GPR) {
    return rv;
  } else {
    auto re = fe->make_gpr(coerce_to_reg_type(m_ts));
    fe->emit(std::make_unique<IR_RegSet>(re, rv));
    return re;
  }
}

/*!
 * Fallback to_xmm if a more optimized one is not provided.
 */
RegVal* Val::to_xmm(Env* fe) {
  auto rv = to_reg(fe);
  if (rv->ireg().kind == emitter::RegKind::XMM) {
    return rv;
  } else {
    assert(false);
    auto re = fe->make_xmm(coerce_to_reg_type(m_ts));
    fe->emit(std::make_unique<IR_RegSet>(re, rv));
    return re;
  }
}

RegVal* RegVal::to_reg(Env* fe) {
  (void)fe;
  return this;
}

RegVal* RegVal::to_gpr(Env* fe) {
  (void)fe;
  if (m_ireg.kind == emitter::RegKind::GPR) {
    return this;
  } else {
    auto re = fe->make_gpr(coerce_to_reg_type(m_ts));
    fe->emit(std::make_unique<IR_RegSet>(re, this));
    return re;
  }
}

RegVal* RegVal::to_xmm(Env* fe) {
  (void)fe;
  if (m_ireg.kind == emitter::RegKind::XMM) {
    return this;
  } else {
    auto re = fe->make_xmm(coerce_to_reg_type(m_ts));
    fe->emit(std::make_unique<IR_RegSet>(re, this));
    return re;
  }
}

RegVal* IntegerConstantVal::to_reg(Env* fe) {
  auto rv = fe->make_gpr(coerce_to_reg_type(m_ts));
  fe->emit(std::make_unique<IR_LoadConstant64>(rv, m_value));
  return rv;
}

RegVal* SymbolVal::to_reg(Env* fe) {
  auto re = fe->make_gpr(coerce_to_reg_type(m_ts));
  fe->emit(std::make_unique<IR_LoadSymbolPointer>(re, m_name));
  return re;
}

RegVal* SymbolValueVal::to_reg(Env* fe) {
  auto re = fe->make_gpr(coerce_to_reg_type(m_ts));
  fe->emit(std::make_unique<IR_GetSymbolValue>(re, m_sym, m_sext));
  return re;
}

RegVal* StaticVal::to_reg(Env* fe) {
  auto re = fe->make_gpr(coerce_to_reg_type(m_ts));
  fe->emit(std::make_unique<IR_StaticVarAddr>(re, obj));
  return re;
}

RegVal* LambdaVal::to_reg(Env* fe) {
  auto re = fe->make_gpr(coerce_to_reg_type(m_ts));
  assert(func);
  fe->emit(std::make_unique<IR_FunctionAddr>(re, func));
  return re;
}

RegVal* InlinedLambdaVal::to_reg(Env* fe) {
  throw std::runtime_error("Cannot put InlinedLambdaVal in a register.");
  return lv->to_reg(fe);
}

RegVal* FloatConstantVal::to_reg(Env* fe) {
  auto re = fe->make_xmm(coerce_to_reg_type(m_ts));
  fe->emit(std::make_unique<IR_StaticVarLoad>(re, m_value));
  return re;
}

RegVal* MemoryOffsetConstantVal::to_reg(Env* fe) {
  auto re = fe->make_gpr(coerce_to_reg_type(m_ts));
  fe->emit(std::make_unique<IR_LoadConstant64>(re, int64_t(offset)));
  fe->emit(std::make_unique<IR_IntegerMath>(IntegerMathKind::ADD_64, re, base->to_gpr(fe)));
  return re;
}

RegVal* MemoryOffsetVal::to_reg(Env* fe) {
  auto re = fe->make_gpr(coerce_to_reg_type(m_ts));
  fe->emit(std::make_unique<IR_RegSet>(re, offset->to_gpr(fe)));
  fe->emit(std::make_unique<IR_IntegerMath>(IntegerMathKind::ADD_64, re, base->to_gpr(fe)));
  return re;
}

RegVal* MemoryDerefVal::to_reg(Env* fe) {
  // todo, support better loads/stores from the stack
  auto base_as_co = dynamic_cast<MemoryOffsetConstantVal*>(base);
  if (base_as_co) {
    auto re = fe->make_gpr(coerce_to_reg_type(m_ts));
    fe->emit(std::make_unique<IR_LoadConstOffset>(re, base_as_co->offset,
                                                  base_as_co->base->to_gpr(fe), info));
    return re;
  } else {
    auto re = fe->make_gpr(coerce_to_reg_type(m_ts));
    auto addr = base->to_gpr(fe);
    fe->emit(std::make_unique<IR_LoadConstOffset>(re, 0, addr, info));
    return re;
  }
}

RegVal* MemoryDerefVal::to_xmm(Env* fe) {
  // todo, support better loads/stores from the stack
  auto base_as_co = dynamic_cast<MemoryOffsetConstantVal*>(base);
  if (base_as_co) {
    auto re = fe->make_xmm(coerce_to_reg_type(m_ts));
    fe->emit(std::make_unique<IR_LoadConstOffset>(re, base_as_co->offset,
                                                  base_as_co->base->to_gpr(fe), info));
    return re;
  } else {
    auto re = fe->make_xmm(coerce_to_reg_type(m_ts));
    auto addr = base->to_gpr(fe);
    fe->emit(std::make_unique<IR_LoadConstOffset>(re, 0, addr, info));
    return re;
  }
}

RegVal* AliasVal::to_reg(Env* fe) {
  auto as_old_type = base->to_reg(fe);
  auto result = fe->make_ireg(m_ts, as_old_type->ireg().kind);
  fe->emit(std::make_unique<IR_RegSet>(result, as_old_type));
  return result;
}

std::string PairEntryVal::print() const {
  if (is_car) {
    return fmt::format("[car of {}]", base->print());
  } else {
    return fmt::format("[cdr of {}]", base->print());
  }
}

RegVal* PairEntryVal::to_reg(Env* fe) {
  int offset = is_car ? -2 : 2;
  auto re = fe->make_gpr(coerce_to_reg_type(m_ts));
  MemLoadInfo info;
  info.reg = RegKind::GPR_64;
  info.sign_extend = true;
  info.size = 4;
  fe->emit(std::make_unique<IR_LoadConstOffset>(re, offset, base->to_gpr(fe), info));
  return re;
}

RegVal* StackVarAddrVal::to_reg(Env* fe) {
  auto re = fe->make_gpr(coerce_to_reg_type(m_ts));
  fe->emit(std::make_unique<IR_GetStackAddr>(re, m_slot));
  return re;
}

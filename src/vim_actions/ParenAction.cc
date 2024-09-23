#include "../vim_actions.h"
ParenAction::ParenAction(std::string symbol, DAction *d) {
    this->symbol = symbol;
    this->d = d;
  }
ActionResult ParenAction::execute(VimMode mode, MotionState &state, Cursor *cursor,
                       Vim *vim){

    return {};
  }
ActionResult ParenAction::peek(VimMode mode, MotionState &state, Cursor *cursor,
                    Vim *vim){
    if (vim->activeAction() || mode == VimMode::VISUAL) {
      state.action = symbol;
      if (mode == VimMode::VISUAL)
        return d->markOnly(mode, state, cursor, vim);
      return withType(ResultType::ExecuteSet);
    }
    return {};
  }
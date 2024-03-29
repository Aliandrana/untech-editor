#include "undostack.h"

using namespace UnTech::Undo;

UndoStack::UndoStack()
    : _undoStack()
    , _redoStack()
    , _dirty(false)
    , _dontMerge(false)
{
}

void UndoStack::add_undo(std::unique_ptr<Action> action)
{
    _undoStack.push_front(std::move(action));

    while (_undoStack.size() > STACK_LIMIT) {
        _undoStack.pop_back();
    }

    _dontMerge = false;

    _redoStack.clear();

    signal_stackChanged.emit();

    markDirty();
}

void UndoStack::add_undoMerge(std::unique_ptr<MergeAction> actionToMerge)
{
    if (_undoStack.empty() || _dontMerge == true || canRedo()) {
        return add_undo(std::move(actionToMerge));
    }

    MergeAction* lastAction = dynamic_cast<UnTech::Undo::MergeAction*>(
        _undoStack.front().get());

    if (lastAction == nullptr) {
        return add_undo(std::move(actionToMerge));
    }

    bool m = lastAction->mergeWith(actionToMerge.get());
    if (m) {
        markDirty();
    }
    else {
        add_undo(std::move(actionToMerge));
    }
}

void UndoStack::dontMergeNextAction()
{
    _dontMerge = true;
}

void UndoStack::undo()
{
    if (canUndo()) {
        _undoStack.front()->undo();

        _redoStack.splice(_redoStack.begin(), _undoStack, _undoStack.begin());

        while (_redoStack.size() > STACK_LIMIT) {
            _redoStack.pop_back();
        }

        signal_stackChanged.emit();

        markDirty();
    }
}

void UndoStack::redo()
{
    if (canRedo()) {
        _redoStack.front()->redo();

        _undoStack.splice(_undoStack.begin(), _redoStack, _redoStack.begin());

        signal_stackChanged.emit();
    }
}

void UndoStack::clear()
{
    _undoStack.clear();
    _redoStack.clear();

    signal_stackChanged.emit();
}

void UndoStack::markDirty()
{
    if (_dirty != true) {
        _dirty = true;
        signal_dirtyChanged.emit();
    }
}

void UndoStack::markClean()
{
    if (_dirty != false) {
        _dirty = false;
        signal_dirtyChanged.emit();
    }
}

static const Glib::ustring emptyString;

const Glib::ustring& UndoStack::getUndoMessage() const
{
    if (!_undoStack.empty()) {
        return _undoStack.front()->message();
    }
    else {
        return emptyString;
    }
}

const Glib::ustring& UndoStack::getRedoMessage() const
{
    if (!_redoStack.empty()) {
        return _redoStack.front()->message();
    }
    else {
        return emptyString;
    }
}

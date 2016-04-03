#include "undostack.h"

using namespace UnTech::Undo;

void UndoStack::add_undo(std::unique_ptr<Action> action)
{
    _undoStack.push_front(std::move(action));

    while (_undoStack.size() > STACK_LIMIT) {
        _undoStack.pop_back();
    }

    _redoStack.clear();

    signal_stackChanged.emit();
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
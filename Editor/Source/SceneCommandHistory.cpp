#include "SceneCommandHistory.h"

#include <utility>

namespace Xelqoria::Editor
{
    void SceneCommandHistory::Reset(SceneCommandHistoryEntry entry)
    {
        m_entries.clear();
        m_entries.push_back(std::move(entry));
        m_currentIndex = 0;
    }

    void SceneCommandHistory::Push(SceneCommandHistoryEntry entry)
    {
        if (m_entries.empty())
        {
            Reset(std::move(entry));
            return;
        }

        if (m_currentIndex + 1 < m_entries.size())
        {
            m_entries.erase(m_entries.begin() + static_cast<std::ptrdiff_t>(m_currentIndex + 1), m_entries.end());
        }

        m_entries.push_back(std::move(entry));
        m_currentIndex = m_entries.size() - 1;
    }

    bool SceneCommandHistory::CanUndo() const
    {
        return !m_entries.empty() && m_currentIndex > 0;
    }

    bool SceneCommandHistory::CanRedo() const
    {
        return !m_entries.empty() && (m_currentIndex + 1) < m_entries.size();
    }

    std::optional<SceneCommandHistoryEntry> SceneCommandHistory::Undo()
    {
        if (!CanUndo())
        {
            return std::nullopt;
        }

        --m_currentIndex;
        return m_entries[m_currentIndex];
    }

    std::optional<SceneCommandHistoryEntry> SceneCommandHistory::Redo()
    {
        if (!CanRedo())
        {
            return std::nullopt;
        }

        ++m_currentIndex;
        return m_entries[m_currentIndex];
    }

    std::optional<SceneCommandHistoryEntry> SceneCommandHistory::GetCurrent() const
    {
        if (m_entries.empty())
        {
            return std::nullopt;
        }

        return m_entries[m_currentIndex];
    }

    std::size_t SceneCommandHistory::GetCount() const
    {
        return m_entries.size();
    }
}

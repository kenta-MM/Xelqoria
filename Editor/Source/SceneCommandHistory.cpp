#include "SceneCommandHistory.h"

#include <utility>
#include <optional>

namespace Xelqoria::Editor
{
    void SceneCommandHistory::Reset(SceneCommandHistoryEntry entry)
    {
        m_entries.clear();
        m_entries.push_back(std::move(entry));
        m_currentIndex = 0;
        m_savedIndex = m_currentIndex;
    }

    bool SceneCommandHistory::Push(SceneCommandHistoryEntry entry)
    {
        if (m_entries.empty())
        {
            Reset(std::move(entry));
            return true;
        }

        if (m_currentIndex + 1 < m_entries.size())
        {
            m_entries.erase(m_entries.begin() + static_cast<std::ptrdiff_t>(m_currentIndex + 1), m_entries.end());
            if (m_savedIndex.has_value() && *m_savedIndex > m_currentIndex)
            {
                m_savedIndex.reset();
            }
        }

        const SceneCommandHistoryEntry& currentEntry = m_entries[m_currentIndex];
        if (currentEntry.serializedScene == entry.serializedScene)
        {
            return false;
        }

        m_entries.push_back(std::move(entry));
        m_currentIndex = m_entries.size() - 1;

        if (m_entries.size() > MaxHistoryEntryCount)
        {
            const std::size_t removeCount = m_entries.size() - MaxHistoryEntryCount;
            m_entries.erase(
                m_entries.begin(),
                m_entries.begin() + static_cast<std::ptrdiff_t>(removeCount));
            m_currentIndex -= removeCount;
            if (m_savedIndex.has_value())
            {
                if (*m_savedIndex < removeCount)
                {
                    m_savedIndex.reset();
                }
                else
                {
                    m_savedIndex = *m_savedIndex - removeCount;
                }
            }
        }

        return true;
    }

    void SceneCommandHistory::MarkSaved()
    {
        if (false == m_entries.empty())
        {
            m_savedIndex = m_currentIndex;
        }
    }

    bool SceneCommandHistory::IsDirty() const
    {
        return false == m_savedIndex.has_value() || *m_savedIndex != m_currentIndex;
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
        if (false == CanUndo())
        {
            return std::nullopt;
        }

        --m_currentIndex;
        return m_entries[m_currentIndex];
    }

    std::optional<SceneCommandHistoryEntry> SceneCommandHistory::Redo()
    {
        if (false == CanRedo())
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

    std::size_t SceneCommandHistory::GetCurrentIndex() const
    {
        return m_currentIndex;
    }
}

#pragma once

#include <Windows.h>
#include <array>
#include <functional>

namespace Xelqoria::Core
{
    /// <summary>
    /// マウスボタンの種類を表す。
    /// </summary>
    enum class MouseButton
    {
        Left
    };

    /// <summary>
    /// 1 フレーム分の低レベル入力状態を表す。
    /// </summary>
    class InputSnapshot
    {
    public:
        static constexpr std::size_t KeyStateCount = 256;

        /// <summary>
        /// 入力がない初期状態のスナップショットを生成する。
        /// </summary>
        InputSnapshot() = default;

        /// <summary>
        /// 収集済みの入力状態からスナップショットを生成する。
        /// </summary>
        /// <param name="keyDownStates">現在フレームのキー押下状態。</param>
        /// <param name="previousKeyDownStates">前フレームのキー押下状態。</param>
        /// <param name="isLeftMouseButtonDown">現在フレームで左マウスボタンが押下中か。</param>
        /// <param name="wasLeftMouseButtonDown">前フレームで左マウスボタンが押下中だったか。</param>
        /// <param name="cursorScreenPoint">スクリーン座標のカーソル位置。</param>
        /// <param name="mouseWheelDelta">現在フレームのマウスホイール差分。</param>
        InputSnapshot(
            const std::array<bool, KeyStateCount>& keyDownStates,
            const std::array<bool, KeyStateCount>& previousKeyDownStates,
            bool isLeftMouseButtonDown,
            bool wasLeftMouseButtonDown,
            POINT cursorScreenPoint,
            int mouseWheelDelta);

        /// <summary>
        /// 指定した仮想キーが現在押下中かを取得する。
        /// </summary>
        /// <param name="virtualKeyCode">Win32 仮想キーコード。</param>
        /// <returns>押下中の場合は true。</returns>
        [[nodiscard]] bool IsKeyDown(int virtualKeyCode) const;

        /// <summary>
        /// 指定した仮想キーが今フレームで押下されたかを取得する。
        /// </summary>
        /// <param name="virtualKeyCode">Win32 仮想キーコード。</param>
        /// <returns>今フレームで押下された場合は true。</returns>
        [[nodiscard]] bool WasKeyPressed(int virtualKeyCode) const;

        /// <summary>
        /// 指定した仮想キーが今フレームで解放されたかを取得する。
        /// </summary>
        /// <param name="virtualKeyCode">Win32 仮想キーコード。</param>
        /// <returns>今フレームで解放された場合は true。</returns>
        [[nodiscard]] bool WasKeyReleased(int virtualKeyCode) const;

        /// <summary>
        /// 指定したマウスボタンが現在押下中かを取得する。
        /// </summary>
        /// <param name="button">確認対象のマウスボタン。</param>
        /// <returns>押下中の場合は true。</returns>
        [[nodiscard]] bool IsMouseButtonDown(MouseButton button) const;

        /// <summary>
        /// 指定したマウスボタンが今フレームで押下されたかを取得する。
        /// </summary>
        /// <param name="button">確認対象のマウスボタン。</param>
        /// <returns>今フレームで押下された場合は true。</returns>
        [[nodiscard]] bool WasMouseButtonPressed(MouseButton button) const;

        /// <summary>
        /// 指定したマウスボタンが今フレームで解放されたかを取得する。
        /// </summary>
        /// <param name="button">確認対象のマウスボタン。</param>
        /// <returns>今フレームで解放された場合は true。</returns>
        [[nodiscard]] bool WasMouseButtonReleased(MouseButton button) const;

        /// <summary>
        /// 現在のカーソル位置をスクリーン座標で取得する。
        /// </summary>
        /// <returns>スクリーン座標のカーソル位置。</returns>
        [[nodiscard]] POINT GetCursorScreenPoint() const;

        /// <summary>
        /// 現在フレームのマウスホイール差分を取得する。
        /// </summary>
        /// <returns>マウスホイール差分。</returns>
        [[nodiscard]] int GetMouseWheelDelta() const;

    private:
        [[nodiscard]] static bool IsValidVirtualKeyCode(int virtualKeyCode);

        std::array<bool, KeyStateCount> m_keyDownStates{};
        std::array<bool, KeyStateCount> m_previousKeyDownStates{};
        bool m_isLeftMouseButtonDown = false;
        bool m_wasLeftMouseButtonDown = false;
        POINT m_cursorScreenPoint{};
        int m_mouseWheelDelta = 0;
    };

    /// <summary>
    /// Win32 入力を毎フレーム収集して InputSnapshot として保持する。
    /// </summary>
    class InputSystem
    {
    public:
        /// <summary>
        /// 仮想キーの押下状態を読み取る関数型を表す。
        /// </summary>
        using KeyStateReader = std::function<bool(int)>;

        /// <summary>
        /// カーソル位置を読み取る関数型を表す。
        /// </summary>
        using CursorPositionReader = std::function<POINT()>;

        /// <summary>
        /// マウスホイール差分を読み取る関数型を表す。
        /// </summary>
        using MouseWheelDeltaReader = std::function<int()>;

        /// <summary>
        /// Win32 API を入力元として InputSystem を生成する。
        /// </summary>
        InputSystem();

        /// <summary>
        /// 任意の入力読み取り関数を使って InputSystem を生成する。
        /// </summary>
        /// <param name="keyStateReader">仮想キー押下状態の読み取り関数。</param>
        /// <param name="cursorPositionReader">カーソル位置の読み取り関数。</param>
        InputSystem(KeyStateReader keyStateReader, CursorPositionReader cursorPositionReader);

        /// <summary>
        /// 任意の入力読み取り関数を使って InputSystem を生成する。
        /// </summary>
        /// <param name="keyStateReader">仮想キー押下状態の読み取り関数。</param>
        /// <param name="cursorPositionReader">カーソル位置の読み取り関数。</param>
        /// <param name="mouseWheelDeltaReader">マウスホイール差分の読み取り関数。</param>
        InputSystem(
            KeyStateReader keyStateReader,
            CursorPositionReader cursorPositionReader,
            MouseWheelDeltaReader mouseWheelDeltaReader);

        /// <summary>
        /// マウスホイール差分を読み取る関数を設定する。
        /// </summary>
        /// <param name="mouseWheelDeltaReader">マウスホイール差分の読み取り関数。</param>
        void SetMouseWheelDeltaReader(MouseWheelDeltaReader mouseWheelDeltaReader);

        /// <summary>
        /// 現在フレームの入力状態を収集する。
        /// </summary>
        void Update();

        /// <summary>
        /// 直近に収集した入力状態を取得する。
        /// </summary>
        /// <returns>入力スナップショット。</returns>
        [[nodiscard]] const InputSnapshot& GetSnapshot() const;

    private:
        KeyStateReader m_keyStateReader{};
        CursorPositionReader m_cursorPositionReader{};
        MouseWheelDeltaReader m_mouseWheelDeltaReader{};
        InputSnapshot m_snapshot{};
    };
}

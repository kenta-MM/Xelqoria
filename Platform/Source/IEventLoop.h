#pragma once

namespace Xelqoria::Platform
{
    /// <summary>
    /// OS のイベントループとメッセージ処理を抽象化する。
    /// </summary>
    class IEventLoop
    {
    public:
        virtual ~IEventLoop() = default;

        /// <summary>
        /// 保留中の OS イベントを 1 回分処理する。
        /// </summary>
        /// <returns>アプリケーション継続中は true。終了要求を受け取った場合は false。</returns>
        virtual bool PumpEvents() = 0;

        /// <summary>
        /// イベントループに終了を要求する。
        /// </summary>
        virtual void RequestQuit() = 0;
    };
}

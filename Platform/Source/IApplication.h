#pragma once

namespace Xelqoria::Platform
{
    /// <summary>
    /// OS 固有のアプリケーション初期化と終了処理を抽象化する。
    /// </summary>
    class IApplication
    {
    public:
        virtual ~IApplication() = default;

        /// <summary>
        /// アプリケーション基盤を初期化する。
        /// </summary>
        /// <returns>初期化に成功した場合は true。</returns>
        virtual bool Initialize() = 0;

        /// <summary>
        /// アプリケーション基盤を終了し、保持している OS リソースを解放する。
        /// </summary>
        virtual void Shutdown() = 0;
    };
}

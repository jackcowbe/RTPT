# RTPT 延伸學習指南

**多執行緒同步・軟體架構・記憶體管理**  
依據 2026-10-28 的 RTPT 原始碼與當日查閱的作者／官方資料整理。

> **建議主線：先完成可解釋的單執行緒渲染迴圈，再做安全的資源上傳，最後加入 CPU 工作執行緒。**
>
> 不必先讀完整套教材才能繼續開發。每次只解決一個問題：誰持有資料、誰正在使用、何時才能修改或回收。

## 閱讀導航

- [1. 你的起點與目前專案](#1-你的起點與目前專案)
- [2. 先分清楚三種同步](#2-先分清楚三種同步)
- [3. 教材選擇與優先順序](#3-教材選擇與優先順序)
- [4. 開源專案怎麼讀](#4-開源專案怎麼讀)
- [5. 把架構與記憶體管理接回 RTPT](#5-把架構與記憶體管理接回-rtpt)
- [6. 閱讀與實作里程碑](#6-閱讀與實作里程碑)
- [7. 現在就能開始的一次學習](#7-現在就能開始的一次學習)

## 1. 你的起點與目前專案

### 程度判斷

依照先前學習背景，你已有自己實作 CPU ray tracing、rasterization 與 C++ 數值程式的經驗；這部分是歷史背景，這次沒有重新檢查那些專案。Unity 整合的 AI 協助經驗，不拿來當作你已能獨立設計所有底層系統的證據。

**我的判斷：你適合從具體渲染器問題學系統程式設計。** 不需要重頭讀一般程式入門，也不宜直接以成熟引擎的整體規模作為實作目標。現在最有價值的是補齊 C++ 生命週期、非同步完成條件，以及資源與工作之間的依賴。

### 本次實際看到的狀態

| 觀察 | 原始碼證據 | 對閱讀安排的影響 |
|---|---|---|
| `main()` 先建立 `Window`，再建立 `VulkanContext` | [main.cpp:6](<D:/Compute_Graphic/Learn Vulkan/RTPT/src/main.cpp:6>) | 可從既有的建立／銷毀順序學所有權 |
| 目前迴圈只處理視窗事件 | [main.cpp:11](<D:/Compute_Graphic/Learn Vulkan/RTPT/src/main.cpp:11>) | 下一步是 command submission 與一幀的生命週期 |
| 使用 `vk-bootstrap` 建立 instance、surface、physical device、device、queues、swapchain | [vulkan_context.cpp:9](<D:/Compute_Graphic/Learn Vulkan/RTPT/src/vulkan_context.cpp:9>) | VKGuide 的路線與現在的工具選擇接近 |
| 取得 swapchain images 與 image views，成功後存入成員 | [vulkan_context.cpp:114](<D:/Compute_Graphic/Learn Vulkan/RTPT/src/vulkan_context.cpp:114>) | 接下來要區分 image、view 與配置記憶體的所有權 |
| 析構時明確銷毀 views、swapchain、device、surface、instance | [vulkan_context.cpp:143](<D:/Compute_Graphic/Learn Vulkan/RTPT/src/vulkan_context.cpp:143>) | 可以接到 RAII、失敗回滾與延後銷毀 |
| C++ 標準為 C++20 | [CMakeLists.txt:4](<D:/Compute_Graphic/Learn Vulkan/RTPT/CMakeLists.txt:4>) | 小練習可使用 C++20，但仍需確認編譯器與標準函式庫支援 |

目前列出的專案程式尚未實作 command pool／buffer、frame fence、渲染提交或 CPU worker。這是**原始碼檢視結果**，沒有編譯、執行或測試 GPU 行為；也不是完整 correctness review。

目前的資料流可以先記成：

```text
main
 ├─ Window ──提供 GLFWwindow*──→ VulkanContext 建立 surface
 └─ VulkanContext
     Instance → Surface → PhysicalDevice → Device / Queues
                                            └→ Swapchain → Images / ImageViews

接下來才加入：取得 image → 錄製 commands → submit → present → 確認可重用
```

圖中的箭頭是簡化的建立／使用依賴，不代表每個物件都有獨立的銷毀動作。`VkPhysicalDevice`、`VkQueue` 這類取得的 handle，不能和自行建立的資源一概而論。

## 2. 先分清楚三種同步

先想像你要上傳一個模型：背景執行緒讀檔，主執行緒拿到頂點，GPU 複製頂點，再由繪製工作讀取。

| 邊界 | 問題 | 會接觸的工具 |
|---|---|---|
| CPU 執行緒 ↔ CPU 執行緒 | 頂點資料是否已完整發布？容器是否有人同時修改？ | `std::mutex`、`std::condition_variable`、join；之後才深入 atomics |
| CPU ↔ GPU | CPU 何時能重寫 staging 區域或重設 command pool？ | fence；進階可用 timeline semaphore 與完成值追蹤 |
| GPU 工作 ↔ GPU 工作 | copy 的結果何時能讓下一個使用者讀到？ | execution／memory dependencies、pipeline barrier、semaphore；image 另考慮 layout |

**CPU 拿到 mutex，不代表 GPU 已經停止讀取；CPU worker 結束，也不代表它產生的 GPU 工作已完成。** Vulkan 的 host threading 與 device 同步是兩套都要成立的條件。[Khronos：Threading](https://docs.vulkan.org/guide/latest/threading.html)

另一個重要區別是「先後執行」與「寫入結果對讀取者可見」。Vulkan 的 execution dependency 與 memory dependency 分別描述這些條件；單靠呼叫順序不足以處理所有資料相依。[Vulkan Specification：Execution and Memory Dependencies](https://docs.vulkan.org/spec/latest/chapters/synchronization.html#synchronization-dependencies)

以下是你的學習模型，不是可直接複製的提交程式：

```text
worker 解碼完成
    ↓ CPU 同步：發布完成資料，主執行緒取得資料
CPU 寫 staging
    ↓ 必要的 host cache 處理與提交規則
GPU copy
    ↓ 對下一個 GPU 使用者建立正確依賴
GPU draw
    ↓ 追蹤涵蓋最後使用者的完成訊號
CPU 才能回收相關資源
```

## 3. 教材選擇與優先順序

以下時間是**第一輪閱讀與筆記的建議時間盒**，不是完成實作所需時間。核心資料都可免費閱讀；付費書只列為補充。

| 優先 | 資料 | 用途 | 建議第一次投入 |
|---|---|---|---|
| A1 現在 | C++ Core Guidelines 選讀 | 所有權、例外安全、鎖的生命週期 | 45–60 分鐘 |
| A2 現在 | VKGuide 選讀＋Khronos 同步指南 | 把現有 swapchain 接成一幀 | 2–3 次，每次 45 分鐘 |
| A3 接著 | OSTEP concurrency 選讀 | 理解 race、等待條件與鎖 | 3 次，每次 45 分鐘 |
| B1 做 buffer 時 | Khronos Memory Allocation＋VMA 文件 | GPU 配置、mapping、回收責任 | 2–3 次，每次 45 分鐘 |
| B2 CPU 練習時 | PBRT 第四版 B.6 | 從渲染工作理解 thread pool | 2 次，每次 45 分鐘 |
| B3 工作量增加時 | Khronos command buffer usage sample | 平行錄製與 pool 配置 | 2 次，每次 45 分鐘 |
| C 有問題再讀 | Game Programming Patterns、PBRT B.4、軟體設計書 | 資料配置與模組界線 | 每次挑一節 |

### A1｜C++ Core Guidelines：先補會影響整個專案的生命週期

**入口：** [作者維護的 Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)。這是設計準則，不是 C++ 語言規格。

第一輪只找：`R.1`、`R.3`、`R.12`、`R.21`、`C.21`。主題是 RAII、借用與持有、立即交付資源管理者、共享所有權的使用時機，以及 copy／move／destructor。

第二輪讀以下四條。`CP` 是 Concurrency and parallelism（並行與平行）的分類代號，數字是準則編號；點連結可跳到原文，也可在原文網頁按 `Ctrl+F` 搜尋 `CP.20:` 等編號。

| 準則與原文連結 | 閱讀重點 |
|---|---|
| [CP.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rconc-raii) | 用 RAII 管理鎖，例如 `std::lock_guard`／`std::unique_lock`，避免手動解鎖遺漏 |
| [CP.22](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rconc-unknown) | 持鎖時避免呼叫行為不明的程式，例如 callback，以免重入或產生鎖相依 |
| [CP.42](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rconc-wait) | 等待 condition variable 時要檢查條件；醒來不代表條件一定成立 |
| [CP.50](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rconc-mutex) | 將 mutex 與它保護的資料放在一起，讓保護關係清楚 |

CP.50 原文提到的 `synchronized_value<T>` 不應當作 C++20 已提供的標準型別；你現在先理解「mutex 與受保護資料一起封裝」即可。

**為什麼適合你：** 每條都能回到目前的 `Window` 或 `VulkanContext` 問具體問題，不必先背整本現代 C++。

**讀完做什麼：** 列出 `Window` 的複製應有什麼語義，以及建構一半失敗時誰負責回收已取得資源。先寫出契約即可。

**先略過：** 大量泛型、coroutine 與 lock-free 規則；遇到相關實作再查。

### A2｜VKGuide：最容易接到你目前進度的實作主線

**入口：** [Rendering Loop](https://vkguide.dev/docs/new_chapter_1/vulkan_mainloop/) → [Improving the render loop](https://vkguide.dev/docs/new_chapter_2/vulkan_new_rendering/)。這裡指 vkguide.dev；與 Khronos 官方 Vulkan Guide 是不同資源。

**只追三件事：** 每幀資料如何分組、CPU 何時等待 GPU、延後清理在哪個完成點執行。它採用的工具與你現在的 `vk-bootstrap` 路線接近，能省去重新對照一整套初始化框架的負擔。

**讀完做什麼：** 畫出 frame slot 0、1 輪流使用 command buffer 的時間線，標明「開始重錄之前等誰」。Deletion queue 只幫你安排清理，仍需正確的 GPU 完成條件。

**使用限制：** 教學是實作參考，精確同步條件回官方資料核對；不要直接照搬 semaphore 的生命週期。下一項是必要搭配。

### A2 的必要搭配｜Khronos：同步指南、範例與 present 重用

**閱讀順序：** [Synchronization](https://docs.vulkan.org/guide/latest/synchronization.html) → [Synchronization Examples](https://docs.vulkan.org/guide/latest/synchronization_examples.html) → [Swapchain Semaphore Reuse](https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html)。

範例先選一種相依，例如「compute 寫 buffer，下一次 compute 讀 buffer」，逐項說明 producer、consumer、stage、access。之後再選與自己 buffer upload 相符的例子，不必一次讀完整頁。

**你現在尤其需要的區別：** frame slot index 與 swapchain image index 不等價。等待 graphics submit 的 fence，不能直接推論 presentation 已用完它等待的 semaphore。官方指南提供依 swapchain image 配置 present-wait semaphore 的重用方式；這會影響未來 frame data 的分組。[官方重用說明](https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html)

**版本門檻：** Synchronization Examples 使用 synchronization2 路線；它進入 Vulkan 1.3 core，或可透過 `VK_KHR_synchronization2` 使用，仍需處理對應 feature 的支援與啟用。你目前的 builder 呼叫沒有明確要求 1.3／相關 features，不能只因 SDK 新就直接假設可用。[Khronos：synchronization2](https://docs.vulkan.org/guide/latest/extensions/VK_KHR_synchronization2.html)

### A3｜OSTEP：讓 mutex 與 condition variable 有因果關係

**入口：** [Operating Systems: Three Easy Pieces，作者免費章節](https://pages.cs.wisc.edu/~remzi/OSTEP/)。優先讀第 26 章 Concurrency、28 章 Locks、30 章 Condition Variables、32 章 Common Concurrency Problems；第 17 章 Free Space Management 留作配置器補充。

**為什麼適合你：** 先理解「執行緒可能在哪裡被切換」，再看同步 API，比直接背 `memory_order` 更容易和實作接上。

**讀完做什麼：** 手畫兩個 worker 同時取出工作時，queue 與停止旗標可能怎樣交錯；列出必須一起保護的不變條件。

**注意：** 書中許多例子使用 C／POSIX API。你在 Windows 的練習可用 C++ 標準同步工具表達同一個概念，不必為閱讀先移植整套 POSIX 環境。OS 的 semaphore 也不能直接當成 Vulkan semaphore 的使用規則。

### B1｜Khronos Memory Allocation＋VMA：理解配置，再利用成熟工具

**入口：** [Khronos Memory Allocation](https://docs.vulkan.org/guide/latest/memory_allocation.html) → VMA [Quick start](https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/quick_start.html) → [Memory mapping](https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/memory_mapping.html) → [General considerations](https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/general_considerations.html)。

**先懂這條關係：** buffer／image 是資源；memory 是儲存配置；binding 把兩者連起來。接著理解 memory type、host-visible／device-local、alignment、suballocation。Khronos 指南解釋為何不宜把每個小資源都對應一次獨立的底層配置。

VMA 適合成為你後續的配置工具，但先讀文件和使用範例，不從大型單一 header 的內部演算法開始。

**讀完做什麼：** 畫出 CPU array → staging allocation → GPU buffer，標出哪些資料能釋放、哪些必須等待 copy 或 draw 完成；區分 mapping、flush／invalidate 與等待 GPU 的不同目的。

**責任邊界：** VMA 不替你安排 GPU copy／barrier 或追蹤最後使用者。Allocator 預設的內部同步，也不代表同一 `VmaAllocation`、其資料存取與 GPU 工作都自動安全。[VMA 的執行緒與功能邊界](https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/general_considerations.html)

### B2｜PBRT 第四版 B.6：最符合你 ray tracing 背景的多執行緒教材

**入口：** [B.6 Parallelism](https://www.pbr-book.org/4ed/Utilities/Parallelism)。第一輪讀 B.6.1 Data Races and Coordination、B.6.4 Thread Pools and Parallel Jobs；再回來看 false sharing 與更進階的原子操作。

**為什麼適合你：** tile／pixel 的工作分配能連回你熟悉的 CPU renderer，讓你把注意力放在工作粒度、共用資料與完成通知。

**讀完做什麼：** 在獨立的小練習把影像分成 tiles；每個 tile 只由一個 worker 寫入，最後等待全部工作完成。先以與單執行緒相同的結果為目標，再量測速度。

**閱讀界線：** 這裡是 CPU parallelism，不是 Vulkan queue 同步。PBRT 的 GPU 路線也不是你目前 Vulkan 專案的直接模板。

### B3｜Khronos Vulkan Samples：需要平行錄製時讀這一個 sample

**入口：** [Command buffer usage and multi-threaded recording](https://docs.vulkan.org/samples/latest/samples/performance/command_buffer_usage/README.html)。

**只追這條路：** draw 工作切分 → worker 取得自己的 pool → 錄製 command buffers → CPU 等工作完成 → 組合與提交 → GPU 完成後回收。

樣本展示 per-frame、per-thread 的資源管理。Command pool 不得被多個執行緒同時使用，即使目標是從它配置出的不同 command buffers；分開 pool 是常用解法。這是主機端的使用契約，不代表每個 CPU worker 都需要一條獨立 GPU queue。

**讀完做什麼：** 畫 2 個 frame slots × 2 個 workers 的 pool 表格，分別標記 CPU 錄製完成與 GPU 執行完成的條件。

**何時值得實作：** 已有可重現的 CPU 錄製成本，再比較單／多執行緒。單一很小的 dispatch 不保證受益；sample 的測量數字也不能直接套用到你的硬體。

### C｜架構與資料配置：依遇到的問題選讀

| 資料 | 推薦讀法 | 接回 RTPT 的問題 |
|---|---|---|
| [Game Programming Patterns：Double Buffer](https://gameprogrammingpatterns.com/double-buffer.html) | 先讀狀態發布與交替 buffer 的概念 | 哪份資料是正在讀的版本、哪份可寫？此模式本身不提供 C++ 或 Vulkan 同步 |
| [Game Programming Patterns：Data Locality](https://gameprogrammingpatterns.com/data-locality.html) | 看連續資料、間接存取與 hot／cold 資料 | 更新大量物件時實際讀了哪些欄位？先量測再改配置 |
| [PBRT B.4：Containers and Memory Management](https://www.pbr-book.org/4ed/Utilities/Containers_and_Memory_Management) | 在能說清楚資料壽命後讀 allocator／scratch buffer 相關部分 | 哪些 CPU 暫存資料能一起回收？不要把 CPU allocator 等同 GPU allocator |
| [A Philosophy of Software Design，作者頁](https://web.stanford.edu/~ouster/cgi-bin/book.php) | 選讀模組深度、資訊隱藏與複雜度；作者頁有第二版資訊及部分摘錄，完整書需另取得 | 拆一個類別後，呼叫者少理解了什麼？沒有必要為開始開發先買書 |

## 4. 開源專案怎麼讀

建議採取「**一個問題、一條呼叫鏈、一張生命週期圖**」的方式。下面列的是實際閱讀入口，不要求全部 clone、編譯或整合；本次也沒有執行這些外部專案。

| 順序 | 專案與入口 | 要追的部分 | 暫時別追的部分 |
|---|---|---|---|
| 1 | [vk-bootstrap：README](https://github.com/charles-lunarg/vk-bootstrap/blob/main/README.md)，搭配本地 `external/vk-bootstrap/src/VkBootstrap.h` | 選你現在用的一個 builder：輸入條件 → result → handle → destroy | 整份裝置選擇與平台相容程式 |
| 2 | [VKGuide 原始碼專案](https://github.com/vblanco20-1/vulkan-guide) | 從上方教學頁提供的對應 source／chapter 入口，看 frame data、draw、cleanup | 不區分新舊教學就直接複製 repository 的任意版本 |
| 3 | [PBRT：parallel.cpp](https://github.com/mmp/pbrt-v4/blob/master/src/pbrt/util/parallel.cpp) | `ParallelFor` → `ParallelForLoop1D::RunStep`，以及 `ThreadPool::Worker`／`WorkOrWait`／析構的停止與 join | 全部 path tracing 演算法、GPU 後端與進階原子技巧 |
| 4 | [Vulkan Samples：command_buffer_usage.cpp](https://github.com/KhronosGroup/Vulkan-Samples/blob/main/samples/performance/command_buffer_usage/command_buffer_usage.cpp) | 找工作切分、thread pool 的工作派送、secondary command buffer 的整合，再往 pool 所屬 frame 追 | 一開始就讀完 framework、UI、scene loader |
| 5 | [VulkanMemoryAllocator：README](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/blob/master/README.md) 與上方官方文件 | allocation 與 buffer 成對建立／銷毀、mapping、thread-safety 契約 | 一開始重寫它的配置演算法 |

**特別留意名稱碰撞：** Vulkan Samples framework 也使用 `vkb` namespace；看到 `vkb::` 不代表一定是你專案用的 `vk-bootstrap` 型別。先找 include 與型別定義，再對照兩個專案。

### 每次讀原始碼都填這張小表

| 問題 | 你要能寫出的答案 |
|---|---|
| 輸入從哪裡來？ | 呼叫者提供的資料、設定與前置條件 |
| 誰擁有、誰借用？ | 負責銷毀者、只可使用者、父物件依賴 |
| 什麼狀態被共同存取？ | 容器、旗標、pool、mapped bytes |
| 什麼證據表示完成？ | CPU join／condition，或 GPU fence／timeline 值，明確標出其涵蓋的工作 |
| 中途失敗呢？ | 已成功取得的資源、回滾順序與錯誤傳遞 |
| 可以何時回收？ | 所有 CPU 與 GPU 使用者都已離開的證明 |

## 5. 把架構與記憶體管理接回 RTPT

以下都是**後續設計建議**，不是已存在的模組，也不是要求你現在改寫架構。

### 先用生命週期決定分組

| 層級 | 可能的內容 | 回收／更新時要問什麼 |
|---|---|---|
| 程式／device 存活期間 | context、allocator、長期共用資源 | 依賴它的工作與物件是否都結束？ |
| swapchain 存活期間 | image views、依 image 管理的 presentation 資源 | resize／重建時，舊 generation 是否仍被使用？ |
| frame slot | command pool、frame fence、該幀暫存配置 | 此 slot 的先前 GPU 工作是否已完成？ |
| frame slot × worker | 平行錄製用的 pools、worker 暫存區 | CPU worker 是否離開？GPU 是否不再使用？ |
| 資產／場景 | mesh buffers、textures | 最後一次使用是否完成？是否還有 CPU 借用？ |

一個 `VulkanContext` 暫時集中初始化，對目前規模是合理起點。等到「每幀重用」「swapchain 重建」「資產上傳」真的有不同的契約，再考慮分開；不必先建立抽象 RHI、ECS 或 render graph。

**架構品質的檢查方式：** 新增一種資源時，你是否能很快回答擁有者、失敗路徑、最後使用者與回收點？類別數量本身無法回答這些問題。

### 三種記憶體問題分開學

| 種類 | 例子 | 第一個學習目標 |
|---|---|---|
| CPU 物件生命週期 | `Window`、`std::vector`、worker 捕捉的資料 | RAII、copy／move、參考失效、工作停止 |
| CPU 資料布局與配置 | tiles、場景陣列、暫存 arena | 存活範圍、連續存取、配置成本、false sharing |
| GPU 配置與存取 | `VkBuffer`／`VkImage`、memory／VMA allocation | memory type、alignment、binding、可見性、最後使用完成 |

`shared_ptr` 的參考計數結束，只能反映相應的 CPU 所有權；你的設計仍需另外追蹤 GPU 使用。將 Vulkan handle 放進有析構的 C++ 類別，也不會自動建立 GPU 完成條件。

### 用現有程式做兩個思考題

1. **`Window` 被複製會怎樣？** [window.h:11](<D:/Compute_Graphic/Learn Vulkan/RTPT/src/window.h:11>) 宣告了析構，卻沒有像 `VulkanContext` 一樣明確刪除 copy。若真的發生預設複製，兩個物件會持有同一 `GLFWwindow*`，目前的析構路徑會各自嘗試銷毀。現有 `main()` 沒有複製它；這是具體的所有權練習，不是聲稱當前流程已觸發錯誤。
2. **建構途中失敗誰清理？** [window.cpp:25](<D:/Compute_Graphic/Learn Vulkan/RTPT/src/window.cpp:25>) 的建窗失敗會 throw；此時 `Window` 尚未完整建構，不能依賴 `~Window()` 去執行先前成功初始化的 GLFW 清理。再對照 `VulkanContext` 各個手動回滾分支，思考用局部資源管理者維持相同契約的方式。先分析，再自己選實作。

這兩題的學習目標是清楚表達責任；並非一定要把每個 Vulkan handle 都包成獨立類別。

## 6. 閱讀與實作里程碑

這是按先決條件排列的選讀路線，不是固定週數的課表。**一次只做目前所在的里程碑**；你自行實作後，再做 read-only review。

| 階段 | 閱讀與工作 | 可驗證的完成條件 |
|---|---|---|
| 0：目前起點 | A1；整理現有物件的所有權與失敗路徑 | 能解釋 Window／Context 的建立、借用與逆序清理 |
| 1：第一個 GPU frame | A2；單執行緒完成取得 image、錄製、提交、present | 能顯示清除色；說明必要 image usage／layout、同步與失敗處理；先不追求三角形 |
| 2：多幀資源重用 | 官方同步與 present 重用指南 | 能分清 frame slot 與 image index；每個 reset／重用都有完成依據；處理 resize／out-of-date 路徑 |
| 3：一條可追蹤的 upload | B1；從 CPU 資料到 GPU buffer | 分清 CPU source、staging、destination 三種壽命；資料正確；回收有 GPU 完成依據 |
| 旁支：CPU 同步小實驗 | A3、B2；獨立於 RTPT 的 tile 工作分配 | 每個 tile 恰好一次、結果等同單執行緒；空 queue、剩餘工作與結束都能正確停止／join |
| 4：有需求才做平行錄製 | B3；先保留單執行緒基準，再增加 workers | CPU 工作不重複／遺漏、pool 無並行衝突；比較 CPU 錄製時間並確認結果正確 |

第一個 frame 的清除色路線，需要選定 swapchain 的使用方式與能力，不能只把 `vkCmdClearColorImage` 接到現有預設 swapchain。讀到實際 API 時再核對完整 usage、layout、stage／access 契約。

**CPU 小實驗的建議形狀：** 一份唯讀輸入、互不重疊的輸出區域、以 mutex／condition variable 保護的工作 queue。先把「等待工作」「停止接新工作」「完成或取消剩餘工作」「喚醒與 join」說清楚，再考慮 lock-free。

**驗證分兩段：** 開發時使用 validation 與 synchronization validation 協助找錯；結果正確後，另以適當的 Release 設定量測效能。驗證層沒有警告不是正確性的完整證明，也無法取代 CPU 執行緒的同步分析。[Khronos：同步驗證入口](https://docs.vulkan.org/guide/latest/synchronization.html)

## 7. 現在就能開始的一次學習

下次先花 **45–60 分鐘**，只做以下內容：

1. 開著目前 `main.cpp`、`window.h/.cpp`、`vulkan_context.h/.cpp`，為每個持有的 handle 記下建立者、擁有者、依賴與清理方式。
2. 選讀 Core Guidelines 的 `R.1` 與 `C.21`，回答前面的 Window 複製與建構失敗兩題。
3. 看 VKGuide 的 Rendering Loop，先畫一張單一 frame slot 的 CPU／GPU 時間線，寫出 command buffer 可重用的條件。

**這一次的產出只需要一張所有權表與一張時間線。** 確認你能用自己的話解釋，再開始 command pool／buffer 的實作。後面的教材保留為開發時的查閱路線。

---

### 資料與查證範圍

- 目前專案結論來自這次讀取的工作目錄內容，沒有以舊的 logical-device 學習紀錄推定現況；也沒有把尚未加入的模組當成既有功能。
- 外部來源採 Khronos、作者網站、原始專案文件／程式碼；連結已於整理當日查閱。章節選擇、時間盒、優先順序與練習是依你程度提出的教學建議。
- 本文件不是完整 API 手冊或規格逐項審查。實作具體 API 時，仍依當時選定的 Vulkan 版本、extensions、features 與官方 Valid Usage 核對。
- 本次只新增學習文件，沒有更動程式、建置設定、安裝函式庫或執行外部 sample。

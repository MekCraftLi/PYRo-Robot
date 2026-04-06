/**
 *******************************************************************************
 * @file    referee-ui.h
 * @brief   裁判系统客户端 UI 绘制服务
 *******************************************************************************
 * @attention
 * 基于 FreeRTOS Queue 实现的无锁异步 UI 绘制接口。
 *******************************************************************************
 * @author  MekLi
 * @date    2026/3/19
 * @version 1.0
 *******************************************************************************
 */

#ifndef INFANTRY_REFEREE_UI_H
#define INFANTRY_REFEREE_UI_H

/* ------- include -----------------------------------------------------------*/
#include "FreeRTOS.h"
#include "System/DataHub/ui-protocol.h"
#include "System/Thread/application-base.h"
#include "queue.h"
#include "tools/crtp.h"

#include <utility>

/* ------- class prototypes --------------------------------------------------*/

class UiRendererSrvc : public PeriodicApp, public Singleton<UiRendererSrvc> {
  public:
    UiRendererSrvc();

    // 流式构建代理类
    class GraphicProxy {
        RMInteractionFigurePayload _payload{};
        bool _commitOnDestruct{true};

      public:
        // 初始化基本属性
        /**
     * @brief 构造函数：初始化通用属性
     * @param name 图形名称 (必须是 3 字节字符串，如 "A01")
     * @param action 默认操作为 Add (增加)
     */
        explicit GraphicProxy(uint8_t name[3], GraphicOption opt = GraphicOption::Add) {
            std::memset(&_payload, 0, sizeof(RMInteractionFigurePayload));

            _payload.graphicName[0] = name[0];
            _payload.graphicName[1] = name[1];
            _payload.graphicName[2] = name[2];

            _payload.opt = static_cast<uint32_t>(opt);

            // 默认合理的初始值
            _payload.color = static_cast<uint32_t>(UiColor::Main);
            _payload.width = 2;
            _payload.layer = 0;
        }

        // 禁用拷贝，允许移动 (现代C++规范)
        GraphicProxy(const GraphicProxy&)            = delete;
        GraphicProxy& operator=(const GraphicProxy&) = delete;
        GraphicProxy(GraphicProxy&& other) noexcept
            : _payload(other._payload), _commitOnDestruct(std::exchange(other._commitOnDestruct, false)) {}

        // --- 魔法析构函数：生命周期结束时自动入队 ---
        ~GraphicProxy() {
            if (_commitOnDestruct) {
                // 这里调用你 ui-renderer-srvc 中的入队函数
                instance()._submitToPipeline(_payload);
            }
        }

        // 通用属性配置 (Fluent API)
        GraphicProxy& color(UiColor c) {
            _payload.color = static_cast<uint32_t>(c);
            return *this;
        }
        GraphicProxy& layer(uint8_t l) {
            _payload.layer = l;
            return *this;
        }
        GraphicProxy& width(uint32_t w) {
            _payload.width = w;
            return *this;
        }
        GraphicProxy& start(uint32_t x, uint32_t y) {
            _payload.startX = x;
            _payload.startY = y;
            return *this;
        }

        // ==========================================
        // 形状专属构造 API (设置 Type 及对应细节参数)
        // ==========================================

        /// @brief 绘制直线 (Line)
        GraphicProxy& asLine(uint16_t endX, uint16_t endY) {
            _payload.type = static_cast<uint32_t>(GraphicType::Line);
            _payload.endX = endX; // detailsD
            _payload.endY = endY; // detailsE
            return *this;
        }

        /// @brief 绘制矩形 (Rectangle)
        GraphicProxy& asRectangle(uint16_t endX, uint16_t endY) {
            _payload.type = static_cast<uint32_t>(GraphicType::Rectangle);
            _payload.endX = endX; // detailsD
            _payload.endY = endY; // detailsE
            return *this;
        }

        /// @brief 绘制正圆 (Circle)
        GraphicProxy& asCircle(uint16_t radius) {
            _payload.type   = static_cast<uint32_t>(GraphicType::Circle);
            _payload.param3 = radius; // detailsC 存放半径
            return *this;
        }

        /// @brief 绘制椭圆 (Ellipse)
        GraphicProxy& asEllipse(uint16_t xSemiAxis, uint16_t ySemiAxis) {
            _payload.type = static_cast<uint32_t>(GraphicType::Ellipse);
            _payload.endX = xSemiAxis; // detailsD
            _payload.endY = ySemiAxis; // detailsE
            return *this;
        }

        /// @brief 绘制圆弧 (Arc)
        GraphicProxy& asArc(uint16_t startAngle, uint16_t endAngle, uint16_t xSemiAxis, uint16_t ySemiAxis) {
            _payload.type   = static_cast<uint32_t>(GraphicType::Arc);
            _payload.param1 = startAngle; // detailsA
            _payload.param2 = endAngle;   // detailsB
            _payload.endX   = xSemiAxis;  // detailsD
            _payload.endY   = ySemiAxis;  // detailsE
            return *this;
        }

        /// @brief 绘制浮点数 (Float)
        GraphicProxy& asFloat(float value, uint16_t fontSize = 20) {
            _payload.type     = static_cast<uint32_t>(GraphicType::Float);
            _payload.param1   = fontSize; // detailsA

            // 浮点数协议：乘以 1000 后拆分进三个位段，共 32 bit
            int32_t scaledVal = static_cast<int32_t>(value * 1000.0f);
            _payload.param3   = scaledVal & 0x3FF;         // detailsC (10 bit)
            _payload.endX     = (scaledVal >> 10) & 0x7FF; // detailsD (11 bit)
            _payload.endY     = (scaledVal >> 21) & 0x7FF; // detailsE (11 bit)
            return *this;
        }

        /// @brief 绘制整数 (Int)
        GraphicProxy& asInt(int32_t value, uint16_t fontSize = 20) {
            _payload.type   = static_cast<uint32_t>(GraphicType::Int);
            _payload.param1 = fontSize; // detailsA

            // 整数协议：原样拆分进三个位段，共 32 bit
            _payload.param3 = value & 0x3FF;         // detailsC (10 bit)
            _payload.endX   = (value >> 10) & 0x7FF; // detailsD (11 bit)
            _payload.endY   = (value >> 21) & 0x7FF; // detailsE (11 bit)
            return *this;
        }

        // 取消本次绘制（如果在复杂逻辑中决定不画了）
        void abort() { _commitOnDestruct = false; }
    };




    /**
         * @brief 启动一个图形绘制流程
         * @param name 图形名称，3 字节
         * @param action 默认是添加(Add)，也可以传入修改(Modify)
         * @return 返回 GraphicProxy，利用链式调用配置参数
         */
    GraphicProxy draw(uint8_t name[3], GraphicOption opt = GraphicOption::Add) {
        return GraphicProxy(name, opt);
    }


    void init() override;
    void run() override;
    static void _sendCustomPacket(uint16_t subCmdId, void* payloadData, uint16_t payloadLen);

    // --- 图形绘制 API (标准 Draw 命名语义) ---
    void drawLine(const GraphicProperties& props, uint16_t endX, uint16_t endY);
    void drawRectangle(const GraphicProperties& props, uint16_t endX, uint16_t endY);
    void drawCircle(const GraphicProperties& props, uint16_t radius);
    void drawEllipse(const GraphicProperties& props, uint16_t radiusX, uint16_t radiusY);
    void drawArc(const GraphicProperties& props, uint16_t radiusX, uint16_t radiusY, uint16_t startAngle,
                 uint16_t endAngle);

    // --- 数值与文本渲染 API ---
    void drawFloat(const GraphicProperties& props, uint16_t fontSize, float value);
    void drawInt(const GraphicProperties& props, uint16_t fontSize, int32_t value);

    // --- 控制 API ---
    void clearGraphic(GraphicDelMode mode, const uint8_t* graphicName = nullptr);

  private:
    QueueHandle_t _renderQueue; // 渲染管线指令队列 (Render Pipeline Queue)

    // 内部封装
    static void _applyProperties(RMInteractionFigurePayload& payload, const GraphicProperties& props, GraphicType type);
    void _submitToPipeline(const RMInteractionFigurePayload& payload); // 提交到渲染管线
};

#endif /* INFANTRY_REFEREE_UI_H */
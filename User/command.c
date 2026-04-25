#include "command.h"

// 双帧头和帧尾
#define COMMAND_HEADER_1 0xAA
#define COMMAND_HEADER_2 0x55
#define COMMAND_TAIL     0x6B

// 循环缓冲区大小 (建议设为256，索引使用uint16_t防止溢出)
#define BUFFER_SIZE 256

uint8_t buffer[BUFFER_SIZE];
uint16_t readIndex = 0;
uint16_t writeIndex = 0;
uint16_t bufferCount = 0; // 引入计数量，初学者最容易理解的防错写法

/**
* @brief 增加读索引，并减少剩余数据量
*/
void Command_AddReadIndex(uint16_t length) {
    readIndex = (readIndex + length) % BUFFER_SIZE;
    bufferCount -= length;
}

/**
* @brief 读取距离当前读指针第 i 位的数据 
* @param i 偏移量
*/
uint8_t Command_Read(uint16_t i) {
    return buffer[(readIndex + i) % BUFFER_SIZE];
}

/**
* @brief 计算未处理的数据长度
*/
uint16_t Command_GetLength() {
    return bufferCount;
}

/**
* @brief 获取缓冲区剩余空间
*/
uint16_t Command_GetRemain() {
    return BUFFER_SIZE - bufferCount;
}

/**
* @brief 向缓冲区写入数据
*/
uint8_t Command_Write(uint8_t *data, uint16_t length) {
    if (Command_GetRemain() < length) {
        return 0; // 空间不足直接丢弃当前碎片
    }
    // 逐字节存入环形缓冲区
    for (uint16_t i = 0; i < length; i++) {
        buffer[writeIndex] = data[i];
        writeIndex = (writeIndex + 1) % BUFFER_SIZE;
    }
    bufferCount += length;
    return length;
}

/**
* @brief 尝试获取一条完整的 Maxicam 指令
* @param command 指令存放指针 (外部需保证数组至少有 10 字节大小)
*/
uint8_t Command_GetCommand(uint8_t *command) {
    for(;;) 
    {
        // 如果未处理的数据长度连一帧 (10字节) 都不够，直接退出等下次串口接收
        if (Command_GetLength() < MAXICAM_FRAME_LENGTH) {
            return 0;
        }

        // 1. 判断第一个包头是否为 0xAA
        if (Command_Read(0) != COMMAND_HEADER_1) {
            Command_AddReadIndex(1); // 扔掉错误的一个字节，继续往后找
            continue;
        }

        // 2. 【修改点2】判断第二个包头是否为 0x55
        if (Command_Read(1) != COMMAND_HEADER_2) {
            // 注意：这里哪怕第二个头不对，也只能扔掉 1 个字节！
            // 因为有可能出现 [0xAA, 0xAA, 0x55...] 的情况，如果你扔掉2个字节，真正的包头就被错过了
            Command_AddReadIndex(1); 
            continue;
        }

        // 3. 提前判断包尾是否为 0x6B (极大地提高筛选效率)
        // 索引为 9 的地方就是第 10 个字节
        if (Command_Read(MAXICAM_FRAME_LENGTH - 1) != COMMAND_TAIL) {
            Command_AddReadIndex(1);
            continue;
        }

        // 4. 【修改点3】计算校验和: 新协议是把 DATA[1] 到 DATA[7] 累加
        uint8_t sum = 0;
        for (uint8_t i = 1; i <= 7; i++) {
            sum += Command_Read(i);
        }

        // 5. 【修改点4】判断算出来的校验和是否等于 DATA[8]
        if (sum != Command_Read(8)) {
            Command_AddReadIndex(1); // 校验失败，说明中间数据坏了，当作废包处理
            continue;
        }

        // 6. 校验全部通过，把这完美的 10 个字节提取到外面的数组里
        for (uint8_t i = 0; i < MAXICAM_FRAME_LENGTH; i++) {
            command[i] = Command_Read(i);
        }

        // 7. 移动读指针一整帧的距离 (10字节)
        Command_AddReadIndex(MAXICAM_FRAME_LENGTH);
        return MAXICAM_FRAME_LENGTH;
    }
}
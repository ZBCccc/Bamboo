#include <gtest/gtest.h>
#include "primitive.h"
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <random>

extern "C"
{
#include <relic/relic.h>
}

using namespace std;

class PiFunctionTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 初始化 RELIC 库
        if (core_get() == nullptr)
        {
            core_init();
            ep_param_set(NIST_P256);
        }
    }

    void TearDown() override
    {
        // 注意：不要在每个测试后清理 RELIC，因为其他测试可能还需要使用
        // core_clean() 应该在程序结束时调用
    }
};

// 测试基本的字符串到椭圆曲线点的映射和逆映射
TEST_F(PiFunctionTest, BasicStringMapping)
{
    ep_t point;
    ep_new(point);

    string original = "hello";
    string recovered;

    // 测试 pi 函数
    pi(point, original);
    EXPECT_FALSE(ep_is_infty(point)) << "pi() should not return point at infinity for valid input";

    // 测试 pi_inv 函数
    pi_inv(recovered, point);
    EXPECT_EQ(original, recovered) << "pi_inv() should recover the original string";

    ep_free(point);
}

// 测试 pi 函数的确定性
TEST_F(PiFunctionTest, DeterministicMapping)
{
    ep_t point1, point2;
    ep_new(point1);
    ep_new(point2);

    string test_string = "deterministic_test";

    // 多次映射相同字符串应该得到相同的椭圆曲线点
    pi(point1, test_string);
    pi(point2, test_string);

    // 比较两个点是否相等
    EXPECT_EQ(RLC_EQ, ep_cmp(point1, point2)) << "Same string should always map to same point";

    ep_free(point1);
    ep_free(point2);
}

// 测试空字符串
TEST_F(PiFunctionTest, EmptyString)
{
    ep_t point;
    ep_new(point);

    string original = "";
    string recovered;

    pi(point, original);
    EXPECT_FALSE(ep_is_infty(point)) << "pi() should handle empty string";

    pi_inv(recovered, point);
    EXPECT_EQ(original, recovered) << "pi_inv() should recover empty string";

    ep_free(point);
}

// 测试单字符字符串
TEST_F(PiFunctionTest, SingleCharacter)
{
    ep_t point;
    ep_new(point);

    string original = "a";
    string recovered;

    pi(point, original);
    EXPECT_FALSE(ep_is_infty(point));

    pi_inv(recovered, point);
    EXPECT_EQ(original, recovered);

    ep_free(point);
}

// 测试长字符串
TEST_F(PiFunctionTest, LongString)
{
    ep_t point;
    ep_new(point);

    string original = "this_is_a_very_long_string_for_testing";
    string recovered;

    pi(point, original);
    EXPECT_FALSE(ep_is_infty(point));

    pi_inv(recovered, point);
    EXPECT_EQ(original, recovered);

    ep_free(point);
}

// 测试特殊字符
TEST_F(PiFunctionTest, SpecialCharacters)
{
    ep_t point;
    ep_new(point);

    vector<string> test_strings = {
        "1file-0",    // Bamboo系统中使用的格式
        "0file-1",    // 另一种格式
        "file@123",   // 包含特殊字符
        "test_123",   // 下划线和数字
        "ABC123xyz",  // 大小写混合
        "!@#$%^&*()", // 特殊符号
    };

    for (const auto &original : test_strings)
    {
        string recovered;

        pi(point, original);
        EXPECT_FALSE(ep_is_infty(point)) << "Failed for string: " << original;

        pi_inv(recovered, point);
        EXPECT_EQ(original, recovered) << "Failed to recover string: " << original;
    }

    ep_free(point);
}

// 测试相同字符串的一致性
TEST_F(PiFunctionTest, ConsistentMapping)
{
    ep_t point1, point2;
    ep_new(point1);
    ep_new(point2);

    string test_string = "consistent_test";

    // 多次映射相同字符串应该得到相同的椭圆曲线点
    pi(point1, test_string);
    pi(point2, test_string);

    // 比较两个点是否相等
    EXPECT_TRUE(ep_cmp(point1, point2) == RLC_EQ) << "Same string should map to same point";

    // 验证两个点都能恢复原始字符串
    string recovered1, recovered2;
    pi_inv(recovered1, point1);
    pi_inv(recovered2, point2);

    EXPECT_EQ(test_string, recovered1);
    EXPECT_EQ(test_string, recovered2);
    EXPECT_EQ(recovered1, recovered2);

    ep_free(point1);
    ep_free(point2);
}

// 测试不同字符串映射到不同点
TEST_F(PiFunctionTest, DifferentStringsMapToDifferentPoints)
{
    ep_t point1, point2;
    ep_new(point1);
    ep_new(point2);

    string string1 = "test1";
    string string2 = "test2";

    pi(point1, string1);
    pi(point2, string2);

    // 不同字符串应该映射到不同的椭圆曲线点
    EXPECT_FALSE(ep_cmp(point1, point2) == RLC_EQ) << "Different strings should map to different points";

    ep_free(point1);
    ep_free(point2);
}

// 测试超长字符串的处理
TEST_F(PiFunctionTest, TooLongString)
{
    ep_t point;
    ep_new(point);

    // 创建一个超过64字节的字符串
    string too_long(65, 'x');

    pi(point, too_long);

    // 应该返回无穷远点表示错误
    EXPECT_TRUE(ep_is_infty(point)) << "pi() should return infinity for too long string";

    ep_free(point);
}

// 测试椭圆曲线运算后的恢复能力
TEST_F(PiFunctionTest, EllipticCurveOperations)
{
    ep_t original_point, modified_point;
    bn_t scalar;
    ep_new(original_point);
    ep_new(modified_point);
    bn_new(scalar);

    string original = "test_ec_ops";
    string recovered;

    // 创建原始映射
    pi(original_point, original);

    // 执行椭圆曲线乘法运算
    bn_t ord;
    bn_new(ord);
    ep_curve_get_ord(ord);
    bn_rand_mod(scalar, ord);

    ep_mul(modified_point, original_point, scalar);

    // 尝试从修改后的点恢复字符串
    pi_inv(recovered, modified_point);

    // 由于椭圆曲线运算改变了点，应该无法恢复原始字符串
    EXPECT_NE(original, recovered) << "pi_inv() should not recover string from modified point";
    EXPECT_TRUE(recovered.empty()) << "pi_inv() should return empty string for unknown point";

    ep_free(original_point);
    ep_free(modified_point);
    bn_free(scalar);
    bn_free(ord);
}

// 测试线程安全性
TEST_F(PiFunctionTest, ThreadSafety)
{
    const int num_threads = 4;
    const int operations_per_thread = 10;

    vector<thread> threads;
    vector<bool> results(num_threads * operations_per_thread, false);

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back([&, t]()
                             {
            for (int i = 0; i < operations_per_thread; ++i) {
                ep_t point;
                ep_new(point);
                
                string original = "thread_" + to_string(t) + "_op_" + to_string(i);
                string recovered;
                
                pi(point, original);
                pi_inv(recovered, point);
                
                results[t * operations_per_thread + i] = (original == recovered);
                
                ep_free(point);
            } });
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    // 检查所有操作是否成功
    for (size_t i = 0; i < results.size(); ++i)
    {
        EXPECT_TRUE(results[i]) << "Thread safety test failed at index " << i;
    }
}

// 测试椭圆曲线加密/解密过程的完整性
TEST_F(PiFunctionTest, EllipticCurveEncryptionDecryption)
{
    ep_t original_point, encrypted_point, decrypted_point;
    bn_t key, inverse_key, order;
    ep_new(original_point);
    ep_new(encrypted_point);
    ep_new(decrypted_point);
    bn_new(key);
    bn_new(inverse_key);
    bn_new(order);

    string original = "1file-42";
    string recovered;

    // 1. 字符串映射到椭圆曲线点
    pi(original_point, original);
    EXPECT_FALSE(ep_is_infty(original_point));

    // 2. 生成随机密钥
    ep_curve_get_ord(order);
    bn_rand_mod(key, order);

    // 3. 椭圆曲线加密 (点乘法)
    ep_mul(encrypted_point, original_point, key);

    // 4. 计算密钥的逆元
    bn_t c, d;
    bn_new(c);
    bn_new(d);
    bn_gcd_ext(c, inverse_key, d, key, order);

    // 5. 椭圆曲线解密
    ep_mul(decrypted_point, encrypted_point, inverse_key);

    // 6. 验证解密后的点与原始点相等
    EXPECT_EQ(RLC_EQ, ep_cmp(original_point, decrypted_point))
        << "Decrypted point should equal original point";

    // 7. 从解密后的点恢复字符串
    pi_inv(recovered, decrypted_point);
    EXPECT_EQ(original, recovered) << "Should recover original string after encryption/decryption";

    // 清理
    ep_free(original_point);
    ep_free(encrypted_point);
    ep_free(decrypted_point);
    bn_free(key);
    bn_free(inverse_key);
    bn_free(order);
    bn_free(c);
    bn_free(d);
}

// 测试性能和大量数据处理
TEST_F(PiFunctionTest, PerformanceAndBulkData)
{
    const int num_tests = 100;
    vector<string> test_strings;

    // 手动分配椭圆曲线点数组
    ep_t *points = new ep_t[num_tests];

    // 生成测试数据
    for (int i = 0; i < num_tests; ++i)
    {
        test_strings.push_back("1file-" + to_string(i));
        ep_new(points[i]);
    }

    auto start = chrono::high_resolution_clock::now();

    // 批量映射
    for (int i = 0; i < num_tests; ++i)
    {
        pi(points[i], test_strings[i]);
        EXPECT_FALSE(ep_is_infty(points[i])) << "Failed at index " << i;
    }

    auto mid = chrono::high_resolution_clock::now();

    // 批量逆映射
    for (int i = 0; i < num_tests; ++i)
    {
        string recovered;
        pi_inv(recovered, points[i]);
        EXPECT_EQ(test_strings[i], recovered) << "Failed to recover at index " << i;
    }

    auto end = chrono::high_resolution_clock::now();

    // 性能统计
    auto mapping_time = chrono::duration_cast<chrono::microseconds>(mid - start);
    auto inverse_time = chrono::duration_cast<chrono::microseconds>(end - mid);

    cout << "Performance results for " << num_tests << " operations:" << endl;
    cout << "  Mapping time: " << mapping_time.count() << " microseconds" << endl;
    cout << "  Inverse time: " << inverse_time.count() << " microseconds" << endl;
    cout << "  Average mapping: " << mapping_time.count() / num_tests << " μs per operation" << endl;
    cout << "  Average inverse: " << inverse_time.count() / num_tests << " μs per operation" << endl;

    // 清理
    for (int i = 0; i < num_tests; ++i)
    {
        ep_free(points[i]);
    }
    delete[] points;
}

// 测试错误输入处理
TEST_F(PiFunctionTest, ErrorHandling)
{
    ep_t point;
    ep_new(point);

    // 测试超长字符串
    string too_long(65, 'x'); // 超过64字节限制
    pi(point, too_long);
    EXPECT_TRUE(ep_is_infty(point)) << "Should return infinity for too long string";

    // 测试从无穷远点恢复字符串
    string recovered;
    ep_set_infty(point);
    pi_inv(recovered, point);
    EXPECT_TRUE(recovered.empty()) << "Should return empty string for infinity point";

    ep_free(point);
}

// 测试随机数据的一致性
TEST_F(PiFunctionTest, RandomDataConsistency)
{
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(1, 50); // 字符串长度1-50

    const int num_random_tests = 50;

    for (int i = 0; i < num_random_tests; ++i)
    {
        // 生成随机字符串
        int length = dis(gen);
        string random_str;
        for (int j = 0; j < length; ++j)
        {
            random_str += static_cast<char>('a' + (gen() % 26));
        }

        ep_t point;
        ep_new(point);

        // 测试映射和逆映射
        pi(point, random_str);
        EXPECT_FALSE(ep_is_infty(point)) << "Failed for random string: " << random_str;

        string recovered;
        pi_inv(recovered, point);
        EXPECT_EQ(random_str, recovered) << "Failed to recover random string: " << random_str;

        ep_free(point);
    }
}

// 全局清理函数
class GlobalCleanup : public ::testing::Environment
{
public:
    void TearDown() override
    {
        // 在所有测试结束后清理 RELIC
        if (core_get() != nullptr)
        {
            core_clean();
        }
    }
};

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    // 注册全局清理
    ::testing::AddGlobalTestEnvironment(new GlobalCleanup);

    return RUN_ALL_TESTS();
}

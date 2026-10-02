"""30类拼音手指字母手势定义 — 最大化传感器区分度

5 ADC通道 (拇指/食指/中指/无名指/小指弯曲度) + 加速度计(手掌朝向) + 陀螺仪(动态轨迹)

设计原则:
- 手指状态: 0=握紧, 0.5=半弯, 1=完全伸展
- 每个手势在 (手指状态 × 手掌朝向) 空间唯一
- D/K/M/N/O/P/Q/R/X 等难区分字母全部自定义简化
- J/Z 用动态手势(陀螺仪轨迹)
"""

# 手掌朝向 (加速度计可区分)
PF = "palm_forward"   # 手掌朝前(远离身体)
PS = "palm_self"      # 手掌朝自己
PD = "palm_down"      # 手掌朝下
PU = "palm_up"        # 手掌朝上
PR = "palm_right"     # 手掌朝右

# (thumb, index, middle, ring, pinky): 0=握紧, 0.5=半弯, 1=伸展
GESTURES = {
    # === 高区分度: 标准手指字母 ===
    "A":  {"fingers": (0, 0, 0, 0, 0), "orient": PF, "desc": "握拳", "type": "static"},
    "B":  {"fingers": (1, 1, 1, 1, 1), "orient": PF, "desc": "五指张开", "type": "static"},
    "C":  {"fingers": (0.5, 0.5, 0.5, 0.5, 0.5), "orient": PF, "desc": "五指微弯呈C形", "type": "static"},
    "D":  {"fingers": (0, 1, 0, 0, 0), "orient": PF, "desc": "食指指天", "type": "static"},
    "E":  {"fingers": (0.5, 0.5, 0.5, 0.5, 0.5), "orient": PS, "desc": "五指微弯朝自己 (区别于C的朝向)", "type": "static"},
    "F":  {"fingers": (1, 1, 1, 0, 0), "orient": PS, "desc": "拇食中三指伸展 (OK手势变体)", "type": "static"},
    "G":  {"fingers": (1, 1, 0, 0, 0), "orient": PR, "desc": "拇食指L形朝右 (枪)", "type": "static"},
    "H":  {"fingers": (0, 1, 1, 0, 0), "orient": PF, "desc": "食中指并拢指前", "type": "static"},
    "I":  {"fingers": (0, 0, 0, 0, 1), "orient": PF, "desc": "小指伸展", "type": "static"},
    "J":  {"fingers": (0, 0, 0, 0, 1), "orient": PF, "desc": "小指画J形 (动态旋转)", "type": "dynamic"},
    "K":  {"fingers": (1, 1, 0, 0, 1), "orient": PF, "desc": "拇食小指伸展 ( horns / 蜘蛛侠 )", "type": "static"},
    "L":  {"fingers": (1, 1, 0, 0, 0), "orient": PS, "desc": "拇食指L形朝自己 (区别于G的朝向)", "type": "static"},
    "M":  {"fingers": (0, 0, 0, 1, 1), "orient": PF, "desc": "无名小指伸展 (替代三指折叠)", "type": "static"},
    "N":  {"fingers": (0, 0, 0, 0, 1), "orient": PS, "desc": "小指伸展朝自己 (区别于I的朝向)", "type": "static"},
    "O":  {"fingers": (0.3, 0.3, 0.3, 0.3, 0.3), "orient": PF, "desc": "指尖捏合呈O形 (中等弯曲,区别于握拳和C形)", "type": "static"},
    "P":  {"fingers": (0, 1, 0, 0, 0), "orient": PD, "desc": "食指指下 (区别于D朝上)", "type": "static"},
    "Q":  {"fingers": (1, 0, 0, 0, 1), "orient": PF, "desc": "拇小指伸展 (区别于K缺食指)", "type": "static"},
    "R":  {"fingers": (0, 0, 1, 0, 0), "orient": PF, "desc": "中指伸展 (替代交叉手势)", "type": "static"},
    "S":  {"fingers": (0, 0, 0, 0, 0), "orient": PD, "desc": "握拳朝下 (区别于A朝前)", "type": "static"},
    "T":  {"fingers": (1, 0, 0, 0, 0), "orient": PF, "desc": "拇指朝上 (赞)", "type": "static"},
    "U":  {"fingers": (0, 1, 0, 0, 1), "orient": PF, "desc": "食小指伸展 (U形)", "type": "static"},
    "V":  {"fingers": (0, 1, 1, 0, 0), "orient": PS, "desc": "食中指V形张开朝自己 (区别于H并拢朝前)", "type": "static"},
    "W":  {"fingers": (0, 1, 1, 1, 0), "orient": PF, "desc": "食中无名三指伸展", "type": "static"},
    "X":  {"fingers": (0, 0.5, 0, 0, 0), "orient": PF, "desc": "食指半弯呈钩 (区别于D全伸)", "type": "static"},
    "Y":  {"fingers": (1, 0, 0, 0, 1), "orient": PS, "desc": "拇小指冲浪手势朝自己 (区别于Q朝前)", "type": "static"},
    "Z":  {"fingers": (0, 1, 0, 0, 0), "orient": PF, "desc": "食指画Z形 (动态轨迹)", "type": "dynamic"},

    # === 双字母专用手势 ===
    "ZH": {"fingers": (1, 1, 1, 0, 0), "orient": PF, "desc": "拇食中三指伸展 (3根)", "type": "static"},
    "CH": {"fingers": (1, 0, 0.5, 0.5, 0), "orient": PF, "desc": "拇指伸展 + 中无名半弯", "type": "static"},
    "SH": {"fingers": (0, 1, 1, 1, 1), "orient": PS, "desc": "四指并拢朝自己 (shhh手势)", "type": "static"},
    "NG": {"fingers": (0, 0, 1, 1, 1), "orient": PF, "desc": "中无小三指伸展", "type": "static"},
}


def check_distinct():
    """验证所有手势在 (finger_states, orientation) 空间唯一"""
    seen = {}
    collisions = []
    for name, g in GESTURES.items():
        # 动态手势不参与静态碰撞检测
        if g["type"] == "dynamic":
            continue
        key = (g["fingers"], g["orient"])
        if key in seen:
            collisions.append(f"  COLLISION: {seen[key]} <-> {name}: {key}")
        else:
            seen[key] = name

    if collisions:
        print("手势冲突:")
        for c in collisions:
            print(c)
    else:
        print("所有手势唯一,无冲突")
    return len(collisions) == 0


def hamming_distance(f1, f2):
    """粗略的二进制汉明距离 + 连续值差异"""
    d = 0
    for a, b in zip(f1, f2):
        d += abs(a - b)
    return d


def check_separation():
    """检查共享手指模式的手势是否有足够朝向分离"""
    static = [(n, g) for n, g in GESTURES.items() if g["type"] == "static"]
    close = []
    for i in range(len(static)):
        for j in range(i + 1, len(static)):
            n1, g1 = static[i]
            n2, g2 = static[j]
            d = hamming_distance(g1["fingers"], g2["fingers"])
            if d < 0.6 and g1["orient"] == g2["orient"]:
                close.append((d, n1, n2))
    close.sort()
    if close:
        print("\n距离过近的手势对 (< 0.6 且同朝向):")
        for d, n1, n2 in close:
            f1 = GESTURES[n1]["fingers"]
            f2 = GESTURES[n2]["fingers"]
            print(f"  {d:.1f}  {n1}{f1} <-> {n2}{f2}")
    else:
        print("所有手势对距离足够")


def print_table():
    """打印手势参考表"""
    print(f"{'字母':<5} {'拇指':<5} {'食指':<5} {'中指':<5} {'无名':<5} {'小指':<5} {'手掌朝向':<14} {'说明'}")
    print("-" * 90)
    for name in sorted(GESTURES.keys(), key=lambda x: (len(x), x)):
        g = GESTURES[name]
        f = g["fingers"]
        t = "动态" if g["type"] == "dynamic" else ""
        orient_short = g["orient"].replace("palm_", "")[:6]
        print(f"{name:<5} {f[0]:<5} {f[1]:<5} {f[2]:<5} {f[3]:<5} {f[4]:<5} {orient_short:<14} {g['desc']} {t}")


def export_label_names():
    """导出 label_names 数组, 供 preprocess.py 使用"""
    names = sorted(GESTURES.keys(), key=lambda x: (len(x), x))
    print("\nlabel_names = [")
    for n in names:
        print(f'    "{n}",')
    print("]")
    return names


if __name__ == '__main__':
    print("=" * 60)
    print("30类拼音手指字母手势定义")
    print("=" * 60)
    print()
    print_table()
    print()
    check_distinct()
    check_separation()
    print()
    print(f"总计: {len(GESTURES)} 类 (26字母 + ZH/CH/SH/NG)")
    print(f"  静态手势: {sum(1 for g in GESTURES.values() if g['type']=='static')}")
    print(f"  动态手势: {sum(1 for g in GESTURES.values() if g['type']=='dynamic')} (J, Z — 轨迹识别)")
    export_label_names()

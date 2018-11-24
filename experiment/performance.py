import os
import time

def shell(cmd):
    print(cmd)
    start = time.time()
    os.system(cmd)
    return time.time() - start

def measure(name, n, MAX, rate=2):
    with open('{}_gen.lmn'.format(name), 'r') as genfile:
        org_genfile = genfile.read()

    with open('{}_log.csv'.format(name), 'w') as logfile:
        logfile.write('n,slim,lmntal-interpreter\n')


    while n <= MAX:
        print("n :", n)

        # PARAMETER を n に書き換え
        with open('{}_gen.lmn'.format(name), 'w') as genfile:
            genfile.write(org_genfile.replace('PARAMETER', str(n)))

        # slim
        shell("cat {}_gen.lmn {}_rule.lmn > {}_slim.lmn".format(name, name, name))
        making_graph_time = shell("slim --use-builtin-rule --hide-ruleset --no-dump {}_gen.lmn > /dev/null".format(name))
        slim_time = shell("slim --use-builtin-rule --hide-ruleset --no-dump {}_slim.lmn > /dev/null".format(name)) - making_graph_time
        
        # lmntal-interpreter
        shell("slim --use-builtin-rule --hide-ruleset {}_gen.lmn > {}_intprt.lmn".format(name, name))
        intprt_parse_time = shell("./lmntal-interpreter < {}_intprt.lmn > /dev/null".format(name))
        shell("cat {}_rule.lmn >> {}_intprt.lmn".format(name, name))
        intprt_time = shell("./lmntal-interpreter < {}_intprt.lmn > /dev/null".format(name)) - intprt_parse_time

        # PARAMETER と書かれた元の状態に戻す
        with open('{}_gen.lmn'.format(name), 'w') as genfile:
            genfile.write(org_genfile)

        with open('{}_log.csv'.format(name), 'a') as logfile:
            logfile.write('{},{:.5f},{:.5f}\n'.format(n, slim_time, intprt_time))

        n = int(n*rate)
        if slim_time > 500.0 or intprt_time > 500.0:
            break

    shell("rm {}_slim.lmn {}_intprt.lmn".format(name, name))

if __name__ == '__main__':
    # measure('aba', 10000, 1000000)
    # measure('aa_a', 10000000, 100000000)
    measure('append', 10000, 20000000)
    # measure('cyclohexane', 1000, 2000, 1.2)
    # measure('cyclohexane_findH', 10, 100, 1.1)




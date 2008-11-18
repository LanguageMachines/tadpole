#! /bin/csh -f
date
get-n-sentences $1 mb-talpa.all.tab > mb-talpa.all.first$1.tab
/home/antalb/Python-2.5.2/python scripts/pairs.py -m20 --feats-bigram  mb-talpa.all.first$1.tab > mb-talpa.all.first$1.inst
/home/antalb/Python-2.5.2/python scripts/dsample.py -r1 mb-talpa.all.first$1.inst > mb-talpa.all.first$1.sampled.inst
rm -rf /exp/antalb/mb-talpa.all.first$1.inst >& /dev/null
Timbl -f mb-talpa.all.first$1.sampled.inst -t mb-talpa.test.inst -a1 +D +vdb+di -o mb-talpa.test.inst.all.first$1.out
/home/antalb/Python-2.5.2/python scripts/dir.py mb-talpa.all.first$1.tab > mb-talpa.all.first$1.dir.inst
Timbl -f mb-talpa.all.first$1.dir.inst -t mb-talpa.test.dir.inst -a1 +D +vdb+di -o mb-talpa.test.dir.all.first$1.out
/home/antalb/Python-2.5.2/python scripts/rels.py mb-talpa.all.first$1.tab > mb-talpa.all.first$1.rels.inst
Timbl -f mb-talpa.all.first$1.rels.inst -t mb-talpa.test.rels.inst -a1 +D +vdb+di -o mb-talpa.test.rels.all.first$1.out

/home/antalb/Python-2.5.2/python scripts/csidp.py -m20 --dep mb-talpa.test.inst.all.first$1.out mb-talpa.test.tab > mb-talpa.test.inst.all.first$1.tab
echo evaluating DEP only...
perl eval.pl -q -p -g mb-talpa.test.tab -s mb-talpa.test.inst.all.first$1.tab

/home/antalb/Python-2.5.2/python scripts/csidp.py -m20 --dep mb-talpa.test.inst.all.first$1.out --dir mb-talpa.test.dir.all.first$1.out mb-talpa.test.tab > mb-talpa.test.all.first$1.main.tab
echo evaluating DEP+DIR...
perl eval.pl -q -p -g mb-talpa.test.tab -s mb-talpa.test.all.first$1.main.tab

/home/antalb/Python-2.5.2/python scripts/csidp.py -m20 --dep mb-talpa.test.inst.all.first$1.out --mod mb-talpa.test.rels.all.first$1.out mb-talpa.test.tab > mb-talpa.test.all.first$1.main.tab
echo evaluating DEP+MOD...
perl eval.pl -q -p -g mb-talpa.test.tab -s mb-talpa.test.all.first$1.main.tab

/home/antalb/Python-2.5.2/python scripts/csidp.py -m20 --dep mb-talpa.test.inst.all.first$1.out --mod mb-talpa.test.rels.all.first$1.out --dir mb-talpa.test.dir.all.first$1.out mb-talpa.test.tab > mb-talpa.test.all.first$1.main.tab
echo evaluating DEP+MOD+DIR...
perl eval.pl -q -p -g mb-talpa.test.tab -s mb-talpa.test.all.first$1.main.tab

date
rm -rf *.all.first$1.* >& /dev/null

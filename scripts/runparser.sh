#! /bin/csh -f
python scripts/pairs.py -m20 --feats-bigram  tadpole.ana > tadpole.ana.inst
python scripts/dsample.py -r1 tadpole.ana.inst > tadpole.ana.sampled.inst
rm -rf tadpole.ana.inst >& /dev/null
python scripts/dir.py tadpole.ana > tadpole.ana.dir.inst
python scripts/rels.py tadpole.ana > tadpole.ana.rels.inst
/home/sloot/usr/local/bin/Timbl -t tadpole.ana.sampled.inst -i config/mb-talpa.all.pairs.sampled.ibase -a1 +D +vdb+di -o tadpole.test.out
/home/sloot/usr/local/bin/Timbl -t tadpole.ana.dir.inst -i config/mb-talpa.all.dir.ibase -a1 +D +vdb+di -o tadpole.test.dir.out
/home/sloot/usr/local/bin/Timbl -t tadpole.ana.rels.inst -i config/mb-talpa.all.rels.ibase -a1 +D +vdb+di -o tadpole.test.rels.out

python scripts/csidp.py -m20 --dep tadpole.test.out --mod tadpole.test.rels.out --dir tadpole.test.dir.out tadpole.ana > tadpole.test.main.tab


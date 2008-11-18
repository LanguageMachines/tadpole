#! /bin/csh -f
python=/home/antalb/Python-2.5.2/python
$python scripts/pairs.py -m20 --feats-bigram  tadpole.ana > tadpole.ana.inst
$python scripts/dir.py tadpole.ana > tadpole.ana.dir.inst
$python scripts/rels.py tadpole.ana > tadpole.ana.rels.inst
/home/sloot/usr/local/bin/Timbl -t tadpole.ana.inst -i config/mbdp-tadpole-alpino.pairs.sampled.ibase -a1 +D +vdb+di -o tadpole.test.out
/home/sloot/usr/local/bin/Timbl -t tadpole.ana.dir.inst -i config/mbdp-tadpole-alpino.dir.ibase -a1 +D +vdb+di -o tadpole.test.dir.out
/home/sloot/usr/local/bin/Timbl -t tadpole.ana.rels.inst -i config/mbdp-tadpole-alpino.rels.ibase -a1 +D +vdb+di -o tadpole.test.rels.out

$python scripts/csidp.py -m20 --dep tadpole.test.out --mod tadpole.test.rels.out --dir tadpole.test.dir.out tadpole.ana > tadpole.ana.result


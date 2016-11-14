for i in `seq 25`; do
	python test.py > tadpoletest.$i.out 2> tadpoletest.$i.err &
done


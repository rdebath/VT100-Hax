



label_1
	nop

label_2
	nop


	dw	label_2 - label_1
	dw	label_2-label_1
	dw	label_2 - label_1, label_2 - label_1
	dw	label_2-label_1,label_2-label_1
	dw	"Hello!"
	dw	"Hello!", "Hello!"
	dw	"Hello!", "Hello!", label_2 - label_1
	dw	"Hello!","Hello!",label_2-label_1
label_3
	dw	"Hello!","Hello!",label_2-label_1
label_4:
	dw	"Hello!","Hello!",label_2-label_1
label5
	dw	"Hello!","Hello!",label_2-label_1
	dw	$, 1








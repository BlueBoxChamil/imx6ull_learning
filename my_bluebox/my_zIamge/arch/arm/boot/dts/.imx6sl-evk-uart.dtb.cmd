cmd_arch/arm/boot/dts/imx6sl-evk-uart.dtb := mkdir -p arch/arm/boot/dts/ ; arm-linux-gnueabihf-gcc -E -Wp,-MD,arch/arm/boot/dts/.imx6sl-evk-uart.dtb.d.pre.tmp -nostdinc -I./arch/arm/boot/dts -I./arch/arm/boot/dts/include -I./drivers/of/testcase-data -undef -D__DTS__ -x assembler-with-cpp -o arch/arm/boot/dts/.imx6sl-evk-uart.dtb.dts.tmp arch/arm/boot/dts/imx6sl-evk-uart.dts ; ./scripts/dtc/dtc -O dtb -o arch/arm/boot/dts/imx6sl-evk-uart.dtb -b 0 -i arch/arm/boot/dts/  -d arch/arm/boot/dts/.imx6sl-evk-uart.dtb.d.dtc.tmp arch/arm/boot/dts/.imx6sl-evk-uart.dtb.dts.tmp ; cat arch/arm/boot/dts/.imx6sl-evk-uart.dtb.d.pre.tmp arch/arm/boot/dts/.imx6sl-evk-uart.dtb.d.dtc.tmp > arch/arm/boot/dts/.imx6sl-evk-uart.dtb.d

source_arch/arm/boot/dts/imx6sl-evk-uart.dtb := arch/arm/boot/dts/imx6sl-evk-uart.dts

deps_arch/arm/boot/dts/imx6sl-evk-uart.dtb := \
  arch/arm/boot/dts/imx6sl-evk.dts \
  arch/arm/boot/dts/include/dt-bindings/gpio/gpio.h \
  arch/arm/boot/dts/include/dt-bindings/input/input.h \
  arch/arm/boot/dts/imx6sl.dtsi \
  arch/arm/boot/dts/include/dt-bindings/interrupt-controller/irq.h \
  arch/arm/boot/dts/include/dt-bindings/interrupt-controller/arm-gic.h \
  arch/arm/boot/dts/skeleton.dtsi \
  arch/arm/boot/dts/imx6sl-pinfunc.h \
  arch/arm/boot/dts/include/dt-bindings/clock/imx6sl-clock.h \

arch/arm/boot/dts/imx6sl-evk-uart.dtb: $(deps_arch/arm/boot/dts/imx6sl-evk-uart.dtb)

$(deps_arch/arm/boot/dts/imx6sl-evk-uart.dtb):

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>      // Required for the GPIO functions
#include <linux/interrupt.h> // Required for the IRQ code
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/kmod.h>

static int device_file_major_number = 0;
static const char device_name[] = "ASO";
static unsigned int buttonRetard = 1000; // ms of button press margin
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eric Planes");
MODULE_DESCRIPTION("Fase 1: ASO 2021-2022");
MODULE_VERSION("1.0");

//List of hardcoded leds
static unsigned int gpioRedLED = 21;    // Purple led (GPIO21)
static unsigned int gpioPurpleLED = 16; // Red led (GPIO16)

//List of hardcoded buttons
static unsigned int gpioButtonRedON = 6;    // Button 1 (GPIO 6)
static unsigned int gpioButtonRedOFF = 0;   // Button 2 (GPIO 0)
static unsigned int gpioButtonPurpON = 11;  // Button 3 (GPIO 11)
static unsigned int gpioButtonPurpOFF = 24; // Button 4 (GPIO 24)

//LED Bools initialized to 0 (off)
static bool ledRed = false;
static bool ledPurple = false;

//Variables per tal d'agafar els nombres IRQ de cada boto
static unsigned int irqRedON;
static unsigned int irqRedOFF;
static unsigned int irqPurpleON;
static unsigned int irqPurpleOFF;

//Variables per tal de contar quantes vegades ha estat premut cada boto
static unsigned int numPressA = 0; //RedON
static unsigned int numPressB = 0; //RedOFF
static unsigned int numPressC = 0; //PurpleON
static unsigned int numPressD = 0; //PurpleOFF

/*
 * És un interruption handler personalitzat que s'activa quan rep una interrupció
 */
static irq_handler_t ebbgpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs)
{
    static char *envp[] = {"HOME=/", "TERM=linux", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL };

    if (irq == irqRedON)
    {
        char * argv[] = { "/bin/bash", "/home/pi/Desktop/Fase1/Scripts/buttonA.sh", NULL};
        call_usermodehelper(argv[0], argv, envp,  UMH_NO_WAIT);
        ledRed = true;
        gpio_set_value(gpioRedLED, ledRed);
        numPressA++;
        printk(KERN_INFO "Fase 1 ASO: Interrupt! LED one (1 - red) ON.\n");
        return (irq_handler_t)IRQ_HANDLED;
    }
    else if (irq == irqRedOFF)
    {
        char * argv[] = { "/bin/bash", "/home/pi/Desktop/Fase1/Scripts/buttonB.sh", NULL};
        call_usermodehelper(argv[0], argv, envp,  UMH_NO_WAIT);
        ledRed = false;
        gpio_set_value(gpioRedLED, ledRed);
        numPressB++;
        printk(KERN_INFO "Fase 1 ASO: Interrupt! LED one (1 - red) OFF.\n");
        return (irq_handler_t)IRQ_HANDLED;
    }
    else if (irq == irqPurpleON)
    {
        char * argv[] = { "/bin/bash", "/home/pi/Desktop/Fase1/Scripts/buttonC.sh", NULL};
        call_usermodehelper(argv[0], argv, envp,  UMH_NO_WAIT);
        ledPurple = true;
        gpio_set_value(gpioPurpleLED, ledPurple);
        numPressC++;
        printk(KERN_INFO "Fase 1 ASO: Interrupt! LED two (1 - purple) ON.\n");
        return (irq_handler_t)IRQ_HANDLED;
    }
    else if (irq == irqPurpleOFF)
    {
        char * argv[] = { "/bin/bash", "/home/pi/Desktop/Fase1/Scripts/buttonD.sh", NULL};
        call_usermodehelper(argv[0], argv, envp,  UMH_NO_WAIT);
        ledPurple = false;
        gpio_set_value(gpioPurpleLED, ledPurple);
        numPressD++;
        printk(KERN_INFO "Fase 1 ASO: Interrupt! LED two (1 - purple) OFF. \n");
        return (irq_handler_t)IRQ_HANDLED;
    }
    else {
        printk(KERN_INFO "Fase 1 ASO: ERROR! An unknown interruption has ocurred with IRQ: %d \n", irq);
    }
    return (irq_handler_t)IRQ_NONE;
}

/*
 * Funcio cridada per my_init i l'utilitzem per inicialitzar els components electronics del mini circuit que hem fet
 */
int init_components(void)
{
    //Informem que s'ha cridat la funcio
    printk(KERN_NOTICE "Fase 1 ASO: init_components() is called\n");

    //Una vegada veiem que tot funciona, configurem el LED numero 1 (vermell)
    ledRed = true;
    gpio_request(gpioRedLED, "ledRed");         //Demanem el LED numero 1
    gpio_direction_output(gpioRedLED, ledRed); //El posem a ON per mostrar que s'ha inicialitzat tot bé
    gpio_export(ledRed, false);                //Fem que aparegui a /sys/class/gpio per tal de poder-lo utilitzar i posem false per a que no desaparegui

    //Per a aquest LED, configurem el botó d'ON i el botó de OFF
    //ON
    gpio_request(gpioButtonRedON, "ledRedBON");
    gpio_direction_input(gpioButtonRedON);   // Fem que sigui un input
    gpio_set_debounce(gpioButtonRedON, buttonRetard); // Li afegim un delay de 200ms (per poder premmer el botó sense que canvii varies vegades d'estat)
    gpio_export(gpioButtonRedON, false);     // Exportem el botó per a que aparegui a /sys/class/gpio i el poguem utilitzar

    //OFF
    gpio_request(gpioButtonRedOFF, "ledRedBOFF");
    gpio_direction_input(gpioButtonRedOFF);   // Fem que sigui un input
    gpio_set_debounce(gpioButtonRedOFF, buttonRetard); // Li afegim un delay de 200ms (per poder premmer el botó sense que canvii varies vegades d'estat)
    gpio_export(gpioButtonRedOFF, false);     // Exportem el botó per a que aparegui a /sys/class/gpio i el poguem utilitzar

    //Ara toca configurar el LED numero 2 (lila)
    ledPurple = true;
    gpio_request(gpioPurpleLED, "ledPurple");            //Demanem el LED numero 1
    gpio_direction_output(gpioPurpleLED, ledPurple); //El posem a ON per mostrar que s'ha inicialitzat tot bé
    gpio_export(ledPurple, false);                   //Fem que aparegui a /sys/class/gpio per tal de poder-lo utilitzar i posem false per a que no desaparegui

    //Per a aquest LED, configurem el botó d'ON i el botó de OFF
    //ON
    gpio_request(gpioButtonPurpON, "ledPurpleBON");
    gpio_direction_input(gpioButtonPurpON);   // Fem que sigui un input
    gpio_set_debounce(gpioButtonPurpON, buttonRetard); // Li afegim un delay de 200ms (per poder premmer el botó sense que canvii varies vegades d'estat)
    gpio_export(gpioButtonPurpON, false);     // Exportem el botó per a que aparegui a /sys/class/gpio i el poguem utilitzar

    //OFF
    gpio_request(gpioButtonPurpOFF, "ledPurpleBOFF");
    gpio_direction_input(gpioButtonPurpOFF);   // Fem que sigui un input
    gpio_set_debounce(gpioButtonPurpOFF, buttonRetard); // Li afegim un delay de 200ms (per poder premmer el botó sense que canvii varies vegades d'estat)
    gpio_export(gpioButtonPurpOFF, false);     // Exportem el botó per a que aparegui a /sys/class/gpio i el poguem utilitzar

    //Informem que s'ha acabat d'inicialitzar tot
    printk(KERN_NOTICE "Fase 1 ASO: components initialized\n");

    return 0;
}

/*
 * Funcio cridada per my_init i l'utilitzem per registrar el device
 */
int register_device(void)
{
    int result = 0;
    printk(KERN_NOTICE "Fase 1 ASO: register_device() is called\n");
    if (result < 0)
    {
        printk(KERN_WARNING "Fase 1 ASO: can\'t register character device with error code = %i\n", result);
        return result;
    }
    device_file_major_number = result;
    printk(KERN_NOTICE "Fase 1 ASO: Device File: %i\n", device_file_major_number);

    //Mirem que tots els leds existeixin en el GPIO
    if (!gpio_is_valid(gpioRedLED))
    {
        //printk(KERN_INFO "Fase 1 ASO: invalid LED number one (1)\n");
        printk(KERN_INFO "Fase 1 ASO: invalid LED number one (red)\n");
        return -ENODEV;
    }

    if (!gpio_is_valid(gpioPurpleLED))
    {
        //printk(KERN_INFO "Fase 1 ASO: invalid LED number two (2)\n");
        printk(KERN_INFO "Fase 1 ASO: invalid LED number two (purple)\n");
        return -ENODEV;
    }
    
    return 0;
}

/*
 * Funcio que es crida quan s'instal·la el LKM
 */
static int __init my_init(void)
{
    int resultA, resultB, resultC, resultD;

    //Missatge que es mostra quan inicialitzem el LKM
    printk(KERN_INFO "Fase 1 ASO: Initializing Fase1 LKM\n");
    register_device();
    init_components();

    //Inicialitzem tots els nums IRQ dels botons
    irqRedON = gpio_to_irq(gpioButtonRedON);
    irqRedOFF = gpio_to_irq(gpioButtonRedOFF);
    irqPurpleON = gpio_to_irq(gpioButtonPurpON);
    irqPurpleOFF = gpio_to_irq(gpioButtonPurpOFF);

    printk(KERN_INFO "Fase 1 ASO: The button A is mapped to IRQ: %d\n", irqRedON);
    printk(KERN_INFO "Fase 1 ASO: The button B is mapped to IRQ: %d\n", irqRedOFF);
    printk(KERN_INFO "Fase 1 ASO: The button C is mapped to IRQ: %d\n", irqPurpleON);
    printk(KERN_INFO "Fase 1 ASO:  The button D is mapped to IRQ: %d\n", irqPurpleOFF);

    // Fem les peticioFase 1 ASO: interrupcions per tal de tractar-les quan toqui
    resultA = request_irq(irqRedON, (irq_handler_t)ebbgpio_irq_handler, IRQF_TRIGGER_RISING, "ebb_gpio_handler", NULL);
    resultB = request_irq(irqRedOFF, (irq_handler_t)ebbgpio_irq_handler, IRQF_TRIGGER_RISING, "ebb_gpio_handler", NULL);
    resultC = request_irq(irqPurpleON, (irq_handler_t)ebbgpio_irq_handler, IRQF_TRIGGER_RISING, "ebb_gpio_handler", NULL);
    resultD = request_irq(irqPurpleOFF, (irq_handler_t)ebbgpio_irq_handler, IRQF_TRIGGER_RISING, "ebb_gpio_handler", NULL);

    return resultA; //Selecciono una de les respostes però de moment no en faig us
}

/*
 * Funcio que es cridaFase 1 ASO quan es desinstal·la el LKM
 */
static void __exit my_exit(void)
{
    printk(KERN_INFO "Fase 1 ASO: The button A was pressed %d times\n", numPressA);
    printk(KERN_INFO "Fase 1 ASO: The button B was pressed %d times\n", numPressB);
    printk(KERN_INFO "Fase 1 ASO: The button C was pressed %d times\n", numPressC);
    printk(KERN_INFO "Fase 1 ASO: The button D was pressed %d times\n", numPressD);

    //Apaguem els LED
    gpio_set_value(gpioRedLED, 0);
    gpio_set_value(gpioPurpleLED, 0);

    //Treiem els botons i els leds de la GPIO
    //Leds
    gpio_unexport(gpioRedLED);
    gpio_unexport(gpioPurpleLED);
    //Botons
    gpio_unexport(gpioButtonRedON);
    gpio_unexport(gpioButtonRedOFF);
    gpio_unexport(gpioButtonPurpON);
    gpio_unexport(gpioButtonPurpOFF);

    //Alliberem memoria
    //Interrupcions
    free_irq(irqRedON, NULL);
    free_irq(irqRedOFF, NULL);
    free_irq(irqPurpleON, NULL);
    free_irq(irqPurpleOFF, NULL);
    //Leds
    gpio_free(gpioRedLED);
    gpio_free(gpioPurpleLED);
    //Botons
    gpio_free(gpioButtonRedON);
    gpio_free(gpioButtonRedOFF);
    gpio_free(gpioButtonPurpON);
    gpio_free(gpioButtonPurpOFF);

    //Informem que s'ha desinstal·lat el LKM
    printk(KERN_INFO "Fase 1 ASO: Goodbye from the LKM!\n");
}

module_init(my_init);
module_exit(my_exit);
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#include "hardware/dma.h"
#include "kiss_fftr.h"

//definitions variables
#define ADC_CHAN 0
#define NB_SAMPLES 1000


//fréquence maximum a lire
#define FRMAX 10000

/*
resolution : 10Hz
durée de mesure : 0.05s

*/

int gpio_adc_chan = 26;

//definition tables
uint8_t tabl_adc_read[NB_SAMPLES];
kiss_fft_scalar fft_in[NB_SAMPLES];
kiss_fft_cpx fft_out[NB_SAMPLES];

//creation du tableau de norme des fft
uint norme_fft[NB_SAMPLES];

//création du tableau de fréquences
uint freqs[NB_SAMPLES];



//definition fonctions
void samplingg(dma_channel_config c, uint chan_reader,uint LED_PIN);
void norme();

int main(){
  stdio_init_all();

  //init builtin led
  const uint LED_PIN = PICO_DEFAULT_LED_PIN;
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  printf("GPIO_INIT\n");

  adc_gpio_init(gpio_adc_chan+ADC_CHAN); //initialisation de l'ADC du canal gpio_adc_chan

  adc_init(); //initialisation de l'ADC
  adc_select_input(ADC_CHAN);

  printf("adc init\n");

  adc_fifo_setup(
        true,    // WEcris tout
        true,    // active les requetes DMA quand le FIFO contiens des données
        1,       // nombre de samples contenus dans le FIFO
        false,   // Inutilse d'avoir un bit d'erreur sur le 15e, on travaille que sur 8 bits
        true     // reduis la taille de chaque sample a 8 bits
    );

  printf("fifo initialisé\n");

  adc_set_clkdiv(9600); // vitesse d'aquisition max

  //DMA setup---------------------------------------------------------------------------------------------------------------------------------

  //setup DMA reader
  uint chan_reader = dma_claim_unused_channel(true);                //reserve un canal DMA
  dma_channel_config c = dma_channel_get_default_config(chan_reader); //recupère la configuration par default du canal
  channel_config_set_transfer_data_size(&c,DMA_SIZE_8);            //le dma va envoyer des baquets d'un octet
  channel_config_set_read_increment(&c, false);                     //lecture du meme endroit
  channel_config_set_write_increment(&c, true);                     //ecriture avec un increment
  channel_config_set_dreq(&c,DREQ_ADC);                             //met la vitesse de transmission du DMA a la vitesse de lecture de l'ADC

  printf("dma setuped\n");

  //execution principale
  while(1){

    //allocation de la memoire
    kiss_fftr_cfg cfg = kiss_fftr_alloc(NB_SAMPLES,false,0,0);
    samplingg(c,chan_reader,LED_PIN); //lecture de l'ADC par le DMA et écriture en mémoire


    //cnversion int en float
    for (int i=0;i<NB_SAMPLES;i++) {
      fft_in[i]=(float)tabl_adc_read[i];
    }

    //effectue la transformée de Fourier des valeurs de l'ADC
    kiss_fftr(cfg,fft_in,fft_out);
    //calcul de la norme de chaque valeur complexe
    norme();
    //libere la mémoire allouée
    kiss_fft_free(cfg);



    //remplis le tableau de fréquences
    float f_max = FRMAX;
    float f_res = f_max / NB_SAMPLES;
    for (int i = 0; i < NB_SAMPLES; i++) {freqs[i] = (uint)f_res*i;}



    //print le tableau de valeurs d'amplitude /!\ prend bcp de temps a faire
    printf("valeurs\n");
    for (int i = 0; i<NB_SAMPLES;i++){
      printf("%d norme %d frequence \n", norme_fft[i], freqs[i]);
    }
    printf("\n\n");


    /*
    //donne le premier indice ou la frequence est >= a 200Hz

    int seuil_fr = 0;
    for(int i = 0; i< NB_SAMPLES;i++){
      if (freqs[i] >= 200){
        seuil_fr = i;
        break;
      }
    }

    //print dans le moniteur serie si une amplitude selectionnée est superieur au seuil_ampl
    //dans le futur, ça va trigger des fonctions pour faire bouger des leds

    int seuil_ampl = 10;
    for (int i = 0; i<seuil_fr; i++){
      if (norme_fft[i]>seuil_ampl){
        print("/n hello there /n");
      }
    }
    */

  }

  }



void samplingg(dma_channel_config c, uint chan_reader, uint LED_PIN){

  adc_fifo_drain();

  adc_run(false);


  dma_channel_configure(
    chan_reader,          //configure le canal chan reader
    &c,                   //configure avec la config pointée par &c
    tabl_adc_read,        //ecrit dans le tableau
    &adc_hw->fifo,        //va chercher les infos du fifo de l'ADC
    NB_SAMPLES,           //transfer 100 octet
    true                  //démarre immediatement
  );

  gpio_put(LED_PIN, 1);
  adc_run(true);
  //bloque l'execution jusqu'à la fin de lecture de l'ADC (1000 samples a 20'000Hz -> 0.05s)
  dma_channel_wait_for_finish_blocking(chan_reader); //prend trop de temps a s'executer (env 0.3s)????????
  adc_fifo_drain();
  gpio_put(LED_PIN, 0);
}

void norme(){
  for (int i = 0 ; i < NB_SAMPLES ; i++){
    //racine de la somme de la partie reelle au carrée et de la partie imaginaire au carré
    norme_fft[i] = (fft_out[i].r*fft_out[i].r)+(fft_out[i].i*fft_out[i].i);
    //norme_fft[i] = (uint8_t)(((uint8_t)fft_out[i].r*(uint8_t)fft_out[i].r)+((uint8_t)fft_out[i].i*(uint8_t)fft_out[i].i));

  }
}

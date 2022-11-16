#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#include "hardware/dma.h"
#include "kiss_fftr.h"

//definitions variables
#define ADC_CHAN 0
#define NB_SAMPLES 100

//fréquence maximum a lire
#define FRMAX 20000

int gpio_adc_chan = 26;

//definition tables
uint8_t tabl_adc_read[NB_SAMPLES];
kiss_fft_scalar fft_in[NB_SAMPLES];
kiss_fft_cpx fft_out[NB_SAMPLES];

//creation du tableau de norme des fft
uint8_t norme_fft[NB_SAMPLES];

//création du tableau de fréquences
float freqs[NB_SAMPLES];



//definition fonctions
void samplingg(dma_channel_config c, uint chan_reader);
void norme();

int main(){
  stdio_init_all();

  adc_gpio_init(gpio_adc_chan+ADC_CHAN); //initialisation de l'ADC du canal gpio_adc_chan

  adc_init(); //initialisation de l'ADC
  adc_select_input(ADC_CHAN);

  adc_fifo_setup(
        true,    // WEcris tout
        true,    // active les requetes DMA quand le FIFO contiens des données
        1,       // nombre de samples contenus dans le FIFO
        false,   // Inutilse d'avoir un bit d'erreur sur le 15e, on travaille que sur 8 bits
        true     // reduis la taille de chaque sample a 8 bits
    );

  adc_set_clkdiv(0); // vitesse d'aquisition max

  //DMA setup

  //setup DMA reader
  uint chan_reader = dma_claim_unused_channel(true);                //reserve un canal DMA
  dma_channel_config c = dma_channel_get_default_config(chan_reader); //recupère la configuration par default du canal
  channel_config_set_transfer_data_size(&c,DMA_SIZE_8);            //le dma va envoyer des baquets d'un octet
  channel_config_set_read_increment(&c, false);                     //lecture du meme endroit
  channel_config_set_write_increment(&c, true);                     //ecriture avec un increment
  channel_config_set_dreq(&c,DREQ_ADC);                             //met la vitesse de transmission du DMA a la vitesse de lecture de l'ADC


  //execution principale
  while(1){
    //allocation de la memoire
    kiss_fftr_cfg cfg = kiss_fftr_alloc(NB_SAMPLES,false,0,0);
    samplingg(c,chan_reader); //lecture de l'ADC par le DMA et écriture en mémoire

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
    for (int i = 0; i < NB_SAMPLES; i++) {freqs[i] = f_res*i;}

    /*
    int seuil_fr = 0;
    for(int i = 0; i< NB_SAMPLES;i++){
      if (freqs[i] >= 200){
        seuil = i;
        break;
      }
    }
    */

    printf("valeurs/n");
    for (int i = 0; i<NB_SAMPLES;i++){
      printf("%d",norme_fft[i]);
    }
    printf("/n/n");

    /*
    int seuil_ampl = 10;
    for (int i = 0; i<seuil_fr; i++){
      if (norme_fft[i]>seuil_ampl){
        print("/n hello there /n");
      }
    }
    */

  }

  }



void samplingg(dma_channel_config c, uint chan_reader){

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

  adc_run(true);
  dma_channel_wait_for_finish_blocking(chan_reader);

}

void norme(){
  for (int i = 0 ; i < NB_SAMPLES ; i++){
    //racine de la somme de la partie reelle au carrée et de la partie imaginaire au carré
    norme_fft[i] = (uint8_t)pow((fft_out[i].r*fft_out[i].r)+(fft_out[i].i*fft_out[i].i),1/2);
  }
}

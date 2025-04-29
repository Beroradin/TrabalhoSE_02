# ✨ Trabalho 02 - Ohmímetro por meio de ADC e I2C

<p align="center"> Repositório dedicado ao Trabalho 02 - SE do processo de capacitação do EmbarcaTech que envolve a implementação de um sistema embarcado que funciona como um ohmímetro, medindo resistências no range de 1k-50k (valores empíricos) na placa Raspberry Pi Pico W por meio da Plataforma BitDogLab.</p>

## :clipboard: Apresentação da tarefa

Para o segundo trabalho da trilha de Sistemas Embarcados (SE) da fase 2 do Embarcatech foi necessário implementar um ohmímetro, que utilizou o ADC (ligado ao canal 2 e retirado o jumper que conecta o mesmo ao microfone da placa BitDogLab), interface de comunicação I2C, botões controlados por interrupções, protoboard, jumpers e resistores. Os resistores são colocados em série e por meio de um divisor de tensão é possível determinar a resistência de um resistor desconhecido (contanto que você saiba a tensão total aplicada e um resistor conhecido). Os dados são exibidos no display OLED por meio de 3 interfaces, uma que exibe o valor da resistência, uma que exibe as faixas de cores do resistor e uma que exibe o valor comercial daquele resistor. Os resistores utilizados são do tipo E24 (5% de tolerância). É exibido por meio do monitor serial os valores em tempo real para uma eventual depuração.

## :dart: Objetivos

- Compreender o funcionamento e a aplicação de conversores analógico-digital em microcontroladores;

- Compreender o funcionamento e a aplicação de interrupções em microcontroladores;

- Implementar a técnica de debouncing via software para eliminar o efeito de bouncing em botões;

- Exibir no display OLED o valor da resistência do resistor medido;

- Exibir no display OLED as faixas de cores esperadas do resistor medido;

- Exibir no display OLED o valor comercial do resistor medido;

- Depuração de dados por meio da interface serial UART.

## :books: Descrição do Projeto

Utilizou-se a placa BitDogLab (que possui o microcontrolador RP2040) para a exibição no dispaly OLED ssd1306 os valores de resistência, faixa de cores e resistência comercial de um resistor. Isso é possível graças ao periférico ADC do RP2040. Os botões intercalam os menus de exibição dos dados do resistor medido. Os botões são controlados por meio de interrupções e possuem debounce via software. Ainda na parte do software, é utilizado técnicas intermediárias como busca linear e struct. O código pode ser depurado via monitor serial. Um adendo sobre o código foi a utilização do valor 9730 para o resistor conhecido (ao invés de 10000, reiterado via multímetro), visto que esse valor seria mais preciso para a medição.

## :walking: Integrantes do Projeto

- Matheus Pereira Alves

## :bookmark_tabs: Funcionamento do Projeto

- O periférico ADC é responsável pela a aquisição dos dados de tensão que são convertidos para valores de resistência;
- O display OLED é capaz de exibir 3 informações sobre o resistor, são elas: Resistência Medida, Faixas de Cores, Resistência Comercial;
- O botão A (GPIO 05) alterna entre os menus de exibição do firmware;
- O botão B (GPIO 06) acessa o modo BootSEL da placa Raspberry Pi Pico W.

## :camera: GIF mostrando o funcionamento do programa na placa BitDogLab
<p align="center">
  <img src="images/trabalho02.gif" alt="GIF" width="526px" />
</p>

## :arrow_forward: Vídeo no youtube mostrando o funcionamento do programa na placa BitDogLab

<p align="center">
    <a href="https://www.youtube.com/watch?v=vHf4V13AaD8">Clique aqui para acessar o vídeo</a>
</p>

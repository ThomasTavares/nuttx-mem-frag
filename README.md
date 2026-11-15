# Segundo Trabalho Prático - Sistemas Operacionais Embarcados

**Autor:** Thomas Tavares Tomaz
**Plataforma:** ESP32-DevKitC (Build FLAT)
**SO:** Apache NuttX (v13.1.0)

---

## 1. Descrição da Funcionalidade Implementada

Foi desenvolvida uma nova chamada de sistema (*syscall*) para o subsistema de gerenciamento de memória do NuttX, nomeada `heap_get_fragmentation_stats()`. O objetivo desta interface é monitorar o estado dos recursos do SO, retornando a porcentagem de fragmentação da memória RAM (*heap*). 

A função calcula a proporção da memória livre que não está contida no maior bloco contíguo disponível, resultando em um índice de 0% (memória totalmente desfragmentada/único bloco) a 100% (memória totalmente dividida em pequenos blocos).

## 2. Arquivos Modificados e Adicionados

As alterações foram divididas entre o kernel do SO e as aplicações de usuário:

**No Kernel do Apache NuttX (`nuttx/`):**
*   **Adicionado:** `mm/mm_heap/mm_fragmentation.c`. Contém a lógica de implementação da chamada de sistema.
*   **Modificado:** `mm/mm_heap/Make.defs`. O arquivo `mm_fragmentation.c` foi incluído na variável `CSRCS` para ser compilado com o sistema.
*   **Modificado:** `include/nuttx/mm/mm.h`. Adicionado o protótipo público da função `size_t heap_get_fragmentation_stats(void);`.

**Na Aplicação de Teste (`apps/`):**
*   **Adicionado:** `examples/fragtest/fragtest_main.c`. Código-fonte da aplicação interativa de teste.
*   **Adicionado:** `examples/fragtest/Kconfig`. Configurações do menu (`make menuconfig`) para a aplicação.
*   **Adicionado:** `examples/fragtest/Make.defs`. Inclusão da pasta no build do repositório `apps`.
*   **Adicionado:** `examples/fragtest/Makefile`. Regras de compilação do executável.

## 3. Estruturas e Funções Internas Utilizadas

Para a implementação sem quebrar o isolamento do SO, as seguintes estruturas internas foram investigadas e utilizadas:

*   **`mm_foreach`:** Função interna nativa do NuttX. Foi utilizada por ser *thread-safe*, travando a região de memória (*mutex*) automaticamente e iterando de forma segura sobre todos os nós do heap.
*   **`FAR struct mm_allocnode_s`:** Estrutura que representa o cabeçalho de um nó (bloco) de memória dentro da iteração.
*   **Macros de inspeção (`MM_NODE_IS_FREE` e `MM_SIZEOF_NODE`):** Utilizadas para verificar os *bits* de metadados de cada nó, permitindo somar os tamanhos apenas dos blocos que estão livres (`total_free`) e registrar qual é o maior deles (`largest_free`).
*   **`g_mmheap` / `g_kmmheap`:** Variáveis globais que apontam para as estruturas base do heap. A implementação suporta o modelo FLAT apontando diretamente para `g_mmheap` (ou `g_kmmheap` dependendo do `.config`).

## 4. Instruções para Compilação e Execução

1. No diretório raiz do NuttX, execute a configuração e habilite a aplicação de teste:
   ```bash
   make menuconfig
   # Navegue até: Application Configuration -> Examples
   # Habilite: [*] RAM Fragmentation Test Example
   ```
2. Compile o sistema e a aplicação executando:
   ```bash
   make
   ```
3. Grave o binário na placa ESP32-DevKitC (ajuste a porta conforme necessário):
   ```bash
   make flash ESPTOOL_PORT=/dev/ttyUSB0 ESPTOOL_BAUD=115200
   ```
4. Acesse o terminal do NuttShell (NSH) via serial (ex: `picocom -b 115200 /dev/ttyUSB0`) e execute a aplicação, passando opcionalmente a quantidade e tamanho dos blocos. Exemplo:
   ```bash
   nsh> fragtest 80 2048
   ```

## 5. Testes Realizados e Resultados Obtidos

O teste `fragtest` aloca blocos dinamicamente e induz "buracos" na memória para estressar o gerenciador do SO e validar o cálculo matemático. Foram executados dois cenários principais de teste na plataforma ESP32-DevKitC:

**Cenário 1: Teste Padrão (Sem argumentos: 40 blocos de 2048 bytes = ~80 KB totais)**

1. **Fragmentação Inicial (~40%):** O teste inicia indicando 40% de fragmentação. Esse resultado reflete o comportamento físico natural do ESP32, que possui múltiplos bancos de memória isolados (SRAM1, SRAM2, RTC). Ao agregá-los, o NuttX não consegue formar um bloco contíguo único da RAM inteira.
2. **Após Alocações (19%):** Ao alocar ~80KB, o espaço é subtraído dos maiores blocos disponíveis. Proporcionalmente, a diferença entre a memória total livre e o maior bloco remanescente diminui, fazendo a fragmentação relativa cair para 19%.
3. **Indução de Buracos (31%):** A liberação exclusiva dos blocos intercalados (pares) devolve ~40KB ao *heap*. Impedidos de se fundir com os blocos vizinhos que seguem alocados, criam-se "buracos" na memória. A chamada de sistema detecta a incapacidade de coalescência e eleva o índice para 31%.
4. **Coalescência e Limpeza (Retorno a ~40%):** Após liberar os blocos restantes, o gerenciador de memória do SO mescla todos os blocos adjacentes. A leitura retorna exatamente ao estado original do *hardware*, comprovando o isolamento da *syscall* e a ausência de *memory leaks*.

**Cenário 2: Teste de Estresse (Comando: `fragtest 150 2048` = ~300 KB totais)**

1. **Fragmentação Inicial (~40%):** Mesmo estado de segmentação de hardware inicial.
2. **Estresse de Alocação (~45%):** Ao requisitar cerca de 300KB (quase toda a RAM livre do microcontrolador), a fragmentação sobe levemente, visto que o pouco espaço que sobra fica limitado às margens dos bancos de memória.
3. **Ápice de Fragmentação (98%):** A liberação intercalada devolve ~150KB distribuídos em "buracos" de 2KB que estão isolados pelos blocos ímpares ainda ativos. O maior bloco livre (`largest_free`) se torna severamente reduzido, restando apenas ~2KB. A chamada de sistema captura esse pico, retornando 98% de fragmentação.
4. **Recuperação Total (Retorno a ~40%):** Na limpeza final, o *heap* sofre a coalescência completa e a fragmentação retorna ao índice inicial, validando novamente a resiliência da iteração feita pela API `mm_foreach`.
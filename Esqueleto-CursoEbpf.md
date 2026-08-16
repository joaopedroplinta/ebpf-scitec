links: [Resumo\_Minicurso\_EBPF](https://docs.google.com/document/d/1jvp5tuTpvVkRrrfnEuDySQ4xpgtcdjjmqCLDFJ4gHws/edit?usp=sharing) | [SlidesCanva-Intro\_eBPF](https://www.canva.com/design/DAHRU2_rajs/pezY60w15-A7ZrtfTFgI9A/edit?utm_content=DAHRU2_rajs&utm_campaign=designshare&utm_medium=link2&utm_source=sharebutton) 

# **1\. Introdução** 

	O mundo em que vivemos atualmente é sustentado por infraestruturas computacionais cada vez mais complexas. Servidores processam milhões de requisições por segundo, redes que transportam volumes gigantescos de dados e aplicações precisam ser monitoradas em tempo real para que falhas sejam detectadas antes de que o usuário perceba.  
	Durante muitos anos, as ferramentas que mantêm um sistema ativo ficam do “lado de fora”, consultando e coletando periodicamente o sistema operacional para coletar algumas métricas relevantes. Mas essa abordagem tem um custo, e além de consumir recursos somos introduzidos a atrasos, e em ambientes de alta demanda, ficamos vulneráveis a competir pelos recursos que deveriam ser apenas monitorados.  
	O Extended Berkeley Packet Filter (eBPF) surge como uma resposta para esse problema. Ele permite que programas sejam executados diretamente dentro do kernel do sistema operacional – o núcleo central do computador – de forma segura, eficiente e mantendo o código fonte sem alterações. Essa característica o torna uma das tecnologias mais relevantes da última década para áreas de observabilidade, segurança de dados e otimização de desempenho (GREGG, 2019).  
	Este curso tem o objetivo de apresentar o eBPF a partir dos seus fundamentos teóricos, arquitetura interna e aplicação prática no monitoramento de sistemas com base em um experimento real conduzindo no contexto de Funções de Rede Virtualizadas (VNFs).

## 	**1.1 O que é eBPF e qual a sua importância**

	Imagine que o kernel de um sistema operacional é como a cozinha de um restaurante: os pedidos chegam, os pratos são preparados e entregues. Os clientes (as aplicações) ficam no salão (lado de fora), e se comunicam com a cozinha por meio de um balcão de pedidos (as chamas de sistema/syscalls).  
	Agora imagine que é de seu interesse monitorar o que acontece nessa cozinha (quantos pratos estão sendo preparados; qual o tempo médio de cada prato; se algum ingrediente está faltando). A abordagem comum seria mandar alguém no salão perguntar periodicamente para o garçom, que se direciona até a cozinha, coleta a informação e volta.   
	O eBPF é diferente, pois ele permite colocar um pequeno observador dentro da própria cozinha, que irá registrar as informações diretamente e as disponibilizar para consulta sempre que necessário. Em termos técnicos, o eBPF é uma tecnologia kernel linux que permite carregar e executar programas em bytecode dentro de uma máquina virtual embutida no próprio kernel.  
	As ferramentas tradicionais de monitoramento, como o Sysstat e o Prometheus, coletam métricas lendo arquivos do pseudossistema /proc – uma interface que o kernel expoẽ no espaço usuário. Essa leitura ocorre periodicamente (polling), o que significa que a ferramenta precisa interromper a atividade que ela estiver executando para ler os dados, processá-los e repetir o ciclo. Sob alta carga, esse processo acaba competindo diretamente pelos recursos da máquina com as aplicações que deveriam apenas ser observadas (CASSAGNES et al., 2020).  
	O eBPF elimina esse problema ao realizar as agregações estatísticas diretamente no kernel, disponibilizando apenas os resultados para o usuário, sem cópias desnecessárias, leituras periódicas de arquivos e latência associada à alternância de kernel e aplicação. Em ambientes de virtualização de funções de rede (NFV), onde cada milissegundo é relevante para decisões de escalonamento automático e recuperação de falhas, essa diferença pode ser determinante (GREGG, 2019;CASSAGNES et al., 2020).

## 	**1.2 O que é o Kernel de um sistema operacional**

	Para entender o eBPF, é preciso entender onde ele está: o kernel. Pense no sistema operacional como uma cidade. Os moradores dessa cidade são programas e aplicações de uso cotidiano. Eles precisam de recursos para funcionar plenamente, como energia, água, estradas, comida, etc (memória RAM, processador, dispositivos de entrada e saída). A prefeitura (kernel) gerencia todos os recursos e define quem tem acesso a quê, quanto e quando. Nenhum morador acessa a infraestrutura diretamente, tudo passa pela prefeitura.  
	Em termos técnicos, o kernel é o núcleo do sistema operacional. Ele é o primeiro programa carregado quando o computador é ligado e permanece em execução enquanto a máquina estiver ativa. É responsável por gerenciar o processador, a memória, os dispositivos de hardware e as comunicações entre os processos. Toda vez que uma aplicação precisa de algo que está fora do seu próprio espaço, ela precisa solicitar ao kernel por meio de uma chamada de sistema (syscall) (Linux Kernel Documentation, 2024).  
	Essa centralização é fundamentada para a segurança e estabilidade do sistema: como o kernel controla todos os acessos aos recursos, ele garante que um programa mal comportado não interfira em outros. É também por isso que modificar o kernel é uma operação delicada, por que um erro pode comprometer todo o sistema. E é nesse momento em que o eBPF se torna relevante, por ser capaz de estender o comportamento do kernel de forma segura e sem fazer alterações em seu código fonte.

## 	**1.3 Espaço Kernel vs Espaço Usuário**

	Uma das distinções mais importantes para compreender o eBPF é a separação entre dois ambientes de execução: o espaço kernel (kernel-space) e o espaço do usuário (user-space).  
	Reutilizando a analogia da cidade, o espaço do usuário pode ser entendido como  bairro residencial e comercial, onde os moradores (aplicações) vivem e trabalham. O espaço do kernel é a área restrita da prefeitura, onde podemos localizar as instalações críticas de infraestrutura (subestação elétrica, reservatório de água, centro de controle de tráfego). Os moradores comuns não entram nessa área, e quando precisam de algo, fazem um pedido formal no balcão de atendimento.  
	 Essa separação existe por razões de segurança e estabilidade. Os programas executados no espaço do usuário têm acesso limitado aos recursos do sistema, já o código executado no espaço kernel, tem acesso irrestrito a tudo. A consequência prática dessa separação é que toda vez que uma ferramenta tradicional (como o Sysstat) quer saber quantos bytes foram trafegados em uma interface de rede, ela precisa sair do espaço do usuário, consultar os contadores do kernel no /proc, trazer esses dados de volta e processá-los.   
	O eBPF resolve esse problema de uma forma elegante, em vez de ficar “indo e voltando” repetidas vezes, ele posiciona um programa dentro do espaço do kernel. Esse programa coleta e agrega os dados no próprio local onde eles são gerados, e só faz a entrega de resultados quando o usuário faz a solicitação – como se a prefeitura passasse a ter um funcionário dedicado que anota tudo e entrega um relatório, sem precisar de interrupções para perguntas (GREGG, 2019; CASSAGNES et al., 2020).

## 	**1.4 Onde o eBPF se aplica**

O eBPF não é uma ferramenta de nicho, ele é uma plataforma de propósito geral que encontrou aplicação em quatro grandes áreas da infraestrutura computacional moderna.

### **1.4.1 Observabilidade**

	A observabilidade é a capacidade de entender o estado interno de um sistema a partir dos dados que ele produz, sejam métricas, logs e rastreamentos. As ferramentas tradicionais de observabilidade dependem de coletas periódicas no espaço do usuário, o que limita a resolução temporal e introduz overhead. O eBPF permite observar o sistema com granularidade de microssegundos, capturando eventos individuais, como a latência de uma única conexão TCP ou o tempo de execução de uma chamada de sistema específica diretamente do kernel (GREGG, 2019).  
	Um exemplo prático: ferramentas como bpftrace e o BPF Compiler Collection (BCC), permitem que um engenheiro de infraestrutura responda perguntas como “quais funções do meu servidor estão consumindo mais CPU?” ou “qual o tempo médio de resposta de conexões na porta 8080?” em tempo real.

### **1.4.2 Segurança**

	No campo da segurança, o eBPF permite implementar políticas de controle de acesso e detecção de comportamentos maliciosos diretamente no kernel, com latência mínima. Em vez de analisar logo após o fato, é possível interceptar chamadas de sistema suspeitas, bloquear conexões de rede em tempo real ou detectar tentativas de escalonamento de privilégios no exato momento em que ocorrem (Linux Documentation, 2024).  
	Uma analogia condizente: se o sistema fosse um aeroporto, as abordagens tradicionais de segurança seriam como revisar as gravações das câmeras horas depois de um acidente. O eBPF seria como ter agentes posicionados em cada ponto de controle, reagindo imediatamente a qualquer comportamento fora do padrão.

### **1.4.3 Redes**

	Na área de redes, o eBPF revolucionou a forma como os pacotes são processados. Com o eXpress Data Path (XDP), é possível tomar decisões sobre esses mesmos pacotes (destacar, encaminhar ou modificar) antes que o sistema operacional aloque estruturas de memória para eles – atingindo taxas da ordem de dezenas de milhões de pacotes por segundo em hardware comum. Isso viabiliza casos de uso como mitigação de ataques DDoS, balanceamento de carga de alto desempenho e roteamento personalizado sem a necessidade de hardware especializado (CASSAGNES et al., 2020).  
	O projeto Cilium (projeto de código aberto que fornece rede, segurança e observabilidade para ambientes nativos da nuvem – como Kubernetes) utiliza eBPF para implementar toda a camada de rede e segurança em clusters Kubernetes, substituindo abordagens baseadas em iptables com ganhos em desempenho e flexibilidade.

### **1.4.4 Performance**

	Por fim, o eBPF é amplamente utilizado para diagnóstico e otimização de desempenho. Engenheiros de sistemas podem instrumentar funções do kernel para identificar gargalos, medir a latência de operações de I/O, rastrear alocações de memória ou detectar contenção em travas (locks), tudo em produção sem a necessidade de recompilar o kernel (GREGG, 2019).  
	Pode-se dizer que: é a diferença entre tentar adivinhar onde está o problema com base em médias agregadas e ter a capacidade de apontar exatamente qual função, qual processo e qual milissegundo causou a degradação.

## 	**1.5 Panorama do mercado e por que essa tecnologia está em alta**

	O interesse pelo eBPF cresceu de forma expressiva nos últimos anos, e não por acaso: ele chegou no momento em que os ambientes computacionais modernos atingiram um nível de complexidade que as ferramentas tradicionais passaram a não conseguir acompanhar com eficiência.  
	A adoção de arquiteturas baseadas em microsserviços, contêineres e orquestração com Kubernetes tornou os sistemas distribuídos mais dinâmicos – os serviços “vêm e vão” em segundos, as instâncias são escalonadas automaticamente, o tráfego de rede entre componentes se tornou tão complexo quanto tráfego externo. Nesse contexto, ferramentas que dependem de agentes instalados manualmente em cada máquina simplesmente não acompanham o ritmo (CASSAGNES et al., 2020).  
	Grandes empresas já colocaram o eBPF no centro de suas infraestruturas. A Meta utiliza eBPF para balanceamento de carga e proteção contra DDoS em escala global. A Cloudflare o emprega na mitigação de ataques na borda da rede. O Google usa eBPF no Google Kubernetes Engine para observabilidade de rede. E o projeto Cilium se tornou o plugin de rede padrão em diversas distribuições Kubernetes empresariais (Linux Kernel Documentation, 2024).  
	Do ponto de vista acadêmico e profissional, o eBPF ainda é uma tecnologia jovem no Brasil, com escassez de material didático em língua portuguesa. Isso acaba por criar uma janela de oportunidade relevante: profissionais e pesquisadores que dominam essa tecnologia hoje estão na vanguarda de uma tendência que tende a se consolidar como padrão de mercado nos próximos anos.  
	Em suma, o eBPF está em alta porque resolve um problema real de tensão entre observabilidade profunda e baixo overhead de uma forma que nenhuma tecnologia anterior havia conseguido com a mesma segurança e estrutura. Não se limita apenas a ser mais uma ferramenta, é uma mudança de paradigma na forma como sistemas operacionais podem ser estendidos e observados.

# **2\. Das origens ao eBPF: contextualização histórica**

	Toda tecnologia carrega a história dos problemas que a antecederam. Para entender por que o eBPF foi criado da forma que foi e por que ele representa uma mudança tão significativa, é preciso voltar ao início: o problema que os engenheiros de redes enfrentavam nos anos 1990 e às limitações que foram se acumulando ao longo dos seguintes anos.

## 	**2.1 Filtragem de pacotes em 1993**

	No início da década de 1990, redes de computadores já geravam volumes consideráveis de tráfego, e havia uma necessidade crescente de capturar e analisar pacotes específicos dentre milhares que transitavam por uma interface de rede. O problema era a forma com que isso seria feito para ser o mais eficiente possível.  
	A abordagem predominante na época copiava todos os pacotes recebidos pela interface de rede para o espaço do usuário, onde um programa aplicava os filtros desejados. O resultado era previsível: um desperdício de recursos, pois a maior parte dos pacotes copiados eram descartados após a filtragem. Era como receber todas as correspondências do seu prédio, abrir cada envelope, ler o remetente e só entregar as suas cartas, jogando fora todo o resto após gastar tempo e energia com elas.  
	Em 1993, Steven McCanne e Van Jacobson, pesquisadores do Lawrence Berkeley National Laboratory, publicaram o artigo “The BSD Packet Filter: A New Architecture for User-Level Packet Capture”, propondo uma solução diferente onde em vez de copiar todos os pacotes para o espaço do usuário e filtrar lá, era executado um filtro diretamente no kernel. (GREGG, 2019).  
	Para isso, eles projetaram uma pequena máquina virtual embutida no kernel (o BPF clássico). Ela possuía uma arquitetura minimalista: dois registradores principais, uma memória de trabalho temporária e um conjunto reduzido de instruções. Programas de filtragem escritos em linguagem de alto nível, como C, eram compilados para o bytecode dessa máquina virtual e executados diretamente no kernel, sem que os pacotes precisassem cruzar o espaço do usuário antes da decisão de filtro. O impacto foi imediato, ferramentas como o tcpdump passaram a usar o BPF como motor de filtragem. E ao mover a decisão de filtragem para dentro do kernel, eliminou-se a cópia desnecessária da grande maioria dos pacotes, reduzindo drasticamente o consumo de CPU e memória nas operações de captura de tráfego (GREGG, 2019).  
	Uma boa analogia para o salto que o BPF representou pode ser exemplificada como: um serviço de triagem de correspondências. Antes do BPF, todo o correio chegava à sua mesa e você mesmo separava o que era seu. Com o BPF, um funcionário especializado no depósito central já separava as correspondências antes do envio, chegando à sua mesa apenas o que era endereçado a você.  
	O BPF clássico foi incorporado ao kernel linux em 1997 e se tornou a base para toda uma geração de ferramentas de análise e segurança de rede. Por mais de duas décadas, ele cumpriu muito bem o seu papel, mas o mundo das redes e dos sistemas distribuídos cresceu em complexidade e já não comporta mais esse modelo.

## 	**2.2 Limitações do BPF clássico**

	O BPF clássico foi uma solução engenhosa para o seu tempo, mas sua arquitetura minimalista começou a mostrar suas juntas à medida que as demandas dos sistemas modernos cresceram.  
	A primeira limitação foi identificada na estrutura, o cBPF possuía apenas dois registradores de 32 bits e uma memória de trabalho fixo. Isso limitava severamente a complexidade dos programas que podiam ser escritos, era possível filtrar pacotes com base em campos do cabeçalho, mas realizar agregações estatísticas, manter o estado entre eventos ou executar lógica mais elaborada estava fora de alcance. Uma analogia é: tentar escrever um romance com um vocabulário de 50 palavras, é possível mas a expressividade é muito restrita.  
	A segunda limitação era o escopo, o BPF foi concebido exclusivamente para filtragem de pacotes de rede. Não havia como utilizá-lo para instrumentar chamadas de sistema, rastrear o comportamento de processos, medir latência de operações de disco ou qualquer outra tarefa de observabilidade além da captura de tráfego. Em um mundo onde os sistemas distribuídos modernos exigem visibilidade sobre dezenas de dimensões diferentes ao mesmo tempo, isso era um gargalo fundamental.  
	A terceira limitação era relacionada ao desempenho, sem compilação Just-In-Time (JIT), os programas BPF eram interpretados instrução por instrução pela máquina virtual, o que introduzia overhead desnecessário em cenários de alto volume de tráfego. Cada pacote precisava passar pelo interpretador, em vez de ser processado por código nativo compilado diretamente na arquitetura do processador.  
	Por fim, havia o problema da portabilidade e da extensibilidade, que ao adicionar novos tipos de filtros ou novas capacidades ao BPF clássico exigia modificar o próprio kernel. O modelo não era projetado para crescer junto com as necessidades dos usuários (GREGG, 2019). Essas limitações, somadas ao crescimento explosivo dos ambientes de nuvem, contêineres e microsserviços ao longo dos anos 2000 e 2010, tornaram evidente  que o BPF precisava ser repaginado, não apenas aprimorado.

## 	**2.3 O redesenho para eBPF em 2014**

	Em 2014, Alexei Starovoitov, engenheiro da equipe de rede do kernel linux, propôs uma reformulação completa do BPF. A proposta não era apenas corrigir as limitações, mas sim transformar em algo fundamentalmente diferente.  
	O resultado foi o Extended BPF, ou eBPF. O conjunto de registradores saltou de dois para onze, todos de 64 bits, o que permitia expressar lógica muito mais complexa dentro de um programa eBPF. A memória de trabalho foi expandida e passou a suportar estruturas de dados sofisticadas, e também, foram introduzidos os mapas eBPF. Esses mapas, são estruturas chave-valor compartilhadas entre o kernel e o espaço do usuário, que pela primeira vez permitiram que programas eBPF mantivessem estado entre execuções e comunicassem resultados ao espaço do usuário de forma assíncrona e eficiente.  
	Outro avanço decisivo foi a adição de um verificador estático (verifier) sofisticado. Antes de qualquer programa eBPF ser carregado no kernel, o verificador analisa o bytecode e garante que ele não contém laços infinitos,que todos os acessos à memória são seguros e que o programa sempre termina. É como um  inspetor rigoroso na porta da cozinha do restaurante, nada entra sem passar pela averiguação. Resolvendo dessa forma a principal preocupação de segurança que sempre acompanhou a ideia de executar código arbitrário dentro do kernel (Linux Kernel Documentation, 2024).  
	A compilação JIT também foi aprimorada e estendida para múltiplas arquiteturas de processador, garantindo que os programas eBPF fossem executados com o desempenho equivalente ao de código nativo compilado diretamente no kernel. Por fim, o eBPF foi desacoplado do domínio exclusivo das redes e passou a ser possível anexar a programas a qualquer parte do kernel. Abrindo desta forma um leque de possibilidades além da filtragem de pacotes. Em 2014, o eBPF foi integrado oficialmente ao kernel linux 3.18, marcando o início de uma nova era na programabilidade de sistemas operacionais (GREGG, 2019).

## 	**2.4 eBPF como motor de programabilidade genérica do kernel**

	Se o BPF clássico era uma ferramenta especializada, o eBPF é uma plataforma. Uma infraestrutura sobre a qual ferramentas especializadas podem ser construídas para finalidades completamente diferentes.   
	Essa mudança de perspectiva é o que torna o eBPF tão significativo. Em vez de modificar kernel para adicionar uma nova funcionalidade, ou escrever um módulo de kernel. Um desenvolvedor pode escrever um programa eBPF, carregá-lo dinamicamente no kernel em execução, e começar a coletar dados ou modificar comportamentos imediatamente, sem a necessidade de reiniciar a máquina (GREGG, 2019; CASSAGNES et al., 2020). Uma analogia: pense no kernel como um celular. Antigamente,  para adicionar uma nova funcionalidade ao telefone, era preciso enviar o aparelho de volta à fábrica para modificar o hardware. Com o eBPF, é como se o celular tivesse ganhado uma loja de aplicativos: você instala o que precisa sem tocar no hardware.  
Essa capacidade de extensão segura e dinâmica do kernel abriu portas para uma geração inteira de ferramentas que hoje fazem parte do cotidiano de engenheiros de infraestrutura em todo o mundo. O bpftrace permite escrever scripts de instrumentação de uma linha para responder perguntas complexas sobre o comportamento do sistema. O Cilium usa eBPF para implementar toda a camada de rede e segurança em clusters Kubernetes. O Falco utiliza eBPF para detectar comportamentos suspeitos em tempo real em ambientes de produção.   
O que todas essas ferramentas têm em comum é que operam dentro do kernel, com acesso direto aos eventos do sistema, sem precisar modificar seu código-fonte e com garantias formais de segurança. Essa combinação é o que define o eBPF como motor de programabilidade genérica do kernel, e é o que justifica seu papel central na infraestrutura computacional moderna (GREGG, 2019; Linux Kernel Documentation, 2024).

# **3\. Fundamentos teóricos do eBPF**

Este capítulo apresenta os componentes internos da tecnologia – a máquina virtual, o conjunto de instruções, o verificador, a compilação JIT, o modelo de portabilidade CO-RE e a biblioteca libbpf.  
Pense como o manual de um motor: você não precisa saber construir um do zero para dirigir bem, mas entender como as peças se encaixam te torna um motorista muito melhor e te prepara para o momento em que algo não funcionar como esperado.

## **3.1 A máquina virtual embutida no kernel**

Uma máquina virtual é um ambiente de execução isolado que simula um processador com seu próprio conjunto de instruções, registradores e memória. Mas que, ao invés de existir em hardware físico, é implementado em software. A VM do eBPF é muito mais compacta e especializada, ela não executa um sistema operacional inteiro, mas sim pequenos programas com uma finalidade específica: coletar dados, tomar decisões sobre pacotes ou reagir a eventos do sistema.  
A grande vantagem de executar código dentro de uma VM, em vez de executá-lo diretamente no kernel, é o isolamento. Um programa que roda diretamente no kernel tem acesso irrestrito a tudo, se ele contiver um erro, pode corromper memória, travar o sistema ou abrir brechas de segurança. Já um programa que roda dentro da VM do eBPF está contido, ele enxerga apenas o que a VM lhe permite enxergar, e qualquer acesso indevido é bloqueado antes de causar dano (Linux Kernel Documentation, 2024).  
Mas o isolamento, por si só, não seria suficiente se viesse acompanhado de lentidão. É aqui que o design da VM do eBPF se destaca: ela foi projetada para ser tão próxima do hardware quanto possível, minimizando o custo de interpretação. A VM do eBPF possui onze registradores de 64 bits, identificados de r0 a r10, cada um com uma função específica (GREGG, 2019):

* r0 armazena o valor de retorno do programa — o resultado que ele comunica ao kernel ao terminar;  
* r1 a r5 são usados para passar argumentos às funções auxiliares (helper functions) que o programa pode chamar;  
* r6 a r9 são registradores de uso geral, preservados entre chamadas de função;  
* r10 é o ponteiro de quadro de pilha (frame pointer), somente leitura, que aponta para a memória de pilha local do programa.

Além dos registradores, cada programa eBPF tem acesso a uma pilha de memória de 512 bytes para armazenamento temporário de variáveis locais. Para dados que precisam persistir entre execuções ou ser compartilhados com o espaço do usuário, o programa utiliza os mapas eBPF.  
Uma analogia para visualizar a VM do eBPF: imagine um funcionário contratado para trabalhar dentro de um banco (o kernel), mas que chegou de uma agência terceirizada. Ele tem acesso ao interior do banco (pode circular pelas áreas restritas, observar as operações, registrar informações) mas trabalha dentro de regras muito claras: não pode abrir cofres sem autorização, não pode modificar registros sem supervisão e, se tentar fazer algo fora das regras, um supervisor (o verificador) o interrompe imediatamente. Ele é útil exatamente porque está lá dentro, mas seguro exatamente porque opera dentro de limites bem definidos (GREGG, 2019; Linux Kernel Documentation, 2024).

## **3.2 Conjunto de instruções**

Todo processador opera com base em um conjunto de instruções: uma lista de operações básicas que ele é capaz de executar. O conjunto de instruções do eBPF define exatamente quais operações um programa pode realizar dentro da VM do kernel. Pense no conjunto de instruções como o vocabulário de uma linguagem. Quanto mais rico o vocabulário, mais coisas você consegue expressar.   
O BPF clássico tinha um vocabulário muito restrito. O eBPF ampliou esse vocabulário de forma significativa, tornando possível escrever programas com lógica complexa, sem abrir mão da segurança e do desempenho (GREGG, 2019).  
O conjunto de instruções do eBPF é organizado em categorias funcionais. As instruções aritméticas e lógicas permitem realizar operações como adição, subtração, multiplicação, divisão, operações bit a bit (AND, OR, XOR, deslocamentos) e negação. Todas operam sobre os registradores de 64 bits da VM. As instruções de movimentação transferem valores entre registradores ou entre registradores e memória. As instruções de acesso à memória permitem ler e escrever dados na pilha local do programa ou nos mapas eBPF, sempre com verificação prévia de limites para garantir que nenhum acesso saia da área autorizada. As instruções de desvio implementam estruturas de controle de fluxo e são a base para toda lógica de tomada de decisão dentro de um programa eBPF. Por fim, as instruções de chamada permitem que o programa invoque helper functions: funções auxiliares fornecidas pelo próprio kernel que expõem funcionalidades seguras ao programa eBPF, como registrar um evento, acessar informações do processo atual ou escrever em um mapa (Linux Kernel Documentation, 2024).  
O eBPF não suporta chamadas de função arbitrárias para dentro do kernel. Um programa eBPF só pode invocar as helper functions explicitamente aprovadas pelo kernel para aquele tipo de programa. Isso é uma decisão de segurança deliberada: ao restringir o conjunto de funções acessíveis, o kernel garante que um programa eBPF não possa, por exemplo, modificar estruturas internas críticas ou acessar memória de outros processos sem autorização. É como contratar um prestador de serviços com acesso ao data center: ele pode usar as ferramentas disponibilizadas pela empresa, mas não pode trazer as suas próprias ou mexer no que não lhe foi autorizado (GREGG, 2019; Linux Kernel Documentation, 2024). Na prática, o desenvolvedor não escreve bytecode eBPF manualmente, ele escreve código em C (ou em linguagens de alto nível como Rust), que é então compilado pelo clang para o bytecode do conjunto de instruções eBPF.

## **3.3 O verificador** 

O verificador (verifier) é o guardião do eBPF. É ele quem garante que nenhum programa malicioso ou mal escrito comprometa a estabilidade do kernel, e é também a razão pela qual o eBPF pode executar código de terceiros dentro do kernel com segurança.  
Toda vez que um programa eBPF é carregado, antes de qualquer execução, o kernel o submete ao verificador. Esse processo acontece uma única vez, em tempo de carregamento, e é completamente transparente para o usuário final. Se o programa passar na verificação, ele é aprovado e carregado; se falhar, é rejeitado com uma mensagem de erro descritiva (Linux Kernel Documentation, 2024).  
O verificador realiza uma análise estática do bytecode, ou seja, ele examina o código sem executá-lo, percorrendo todos os caminhos possíveis de execução. Essa análise verifica três categorias principais de propriedades.  
A primeira é a terminação garantida: o verificador garante que o programa sempre termina, sem laços infinitos. Para isso, ele impõe um limite máximo de instruções que um programa pode executar. Se um programa contiver um laço que potencialmente nunca termina, o verificador o rejeita. Essa restrição é fundamental: um programa que nunca termina travaria o kernel inteiro ao ser acionado por um evento de rede de alto volume (GREGG, 2019).  
A segunda é a segurança de memória: o verificador rastreia o tipo e os limites de cada ponteiro usado pelo programa. Antes de qualquer acesso à memória, ele verifica se o ponteiro aponta para uma região válida e se os limites do acesso estão dentro da área autorizada. Um acesso fora dos limites é rejeitado em tempo de verificação, antes mesmo de o programa rodar (Linux Kernel Documentation, 2024).  
A terceira é a validação de tipos: o verificador mantém controle do tipo de cada registrador ao longo de todos os caminhos de execução. Se um programa tenta usar um valor de retorno de uma helper function sem antes verificar se ele é nulo (null check), o verificador rejeita o programa, porque um acesso a um ponteiro nulo causaria falha catastrófica no kernel.  
Uma analogia: imagine um revisor jurídico que analisa um contrato antes de ele ser assinado. Ele não espera o contrato ser executado para descobrir que uma cláusula é inválida, ele lê tudo com antecedência, identifica os problemas e devolve o documento para correção antes que qualquer consequência ocorra.   
Vale destacar que o verificador é também o principal responsável pela curva de aprendizado do eBPF, suas mensagens de erro podem ser críticas para quem está começando, e alguns padrões de código perfeitamente válidos em C são rejeitados porque o verificador não consegue provar estaticamente que são seguros.

## **3.4 Compilação Just-In-Time (JIT)**

Após passar pelo verificador, o programa eBPF em bytecode precisa ser executado de alguma forma pelo processador físico da máquina.  
Na interpretação, a VM leria cada instrução do bytecode uma por uma e executaria a operação correspondente. Essa abordagem é flexível, mas lenta, para cada instrução do programa, são necessárias várias instruções do processador físico para interpretar e executar a operação.  
Na compilação Just-In-Time (JIT), o bytecode é traduzido para instruções nativas do processador físico no momento do carregamento, ou seja, "bem na hora" de ser usado. O resultado é que o programa passa a existir como código de máquina nativo, executado diretamente pelo processador sem nenhuma camada de interpretação no meio. O overhead de tradução ocorre uma única vez, no carregamento, e todas as execuções subsequentes rodam à velocidade máxima do hardware (GREGG, 2019).  
Uma analogia: imagine um intérprete simultâneo em uma conferência internacional. A interpretação simultânea introduz um atraso constante, o equivalente à interpretação de bytecode. A compilação JIT seria como ter o discurso traduzido e impresso com antecedência: na hora da apresentação, o orador já lê diretamente na língua do público, sem nenhum atraso de tradução.  
No eBPF, o compilador JIT do kernel traduz o bytecode verificado para o conjunto de instruções nativo da arquitetura do processador em uso. Isso significa que o mesmo bytecode eBPF, compilado uma única vez, pode ser carregado em máquinas com arquiteturas diferentes e será traduzido para o código nativo de cada uma delas pelo JIT local (Linux Kernel Documentation, 2024).  
O ganho de desempenho é expressivo. Benchmarks reportados na literatura mostram que programas eBPF com JIT habilitado têm desempenho comparável ao de módulos de kernel nativos para operações de rede, enquanto a versão interpretada pode ser de quatro a dez vezes mais lenta dependendo da carga de trabalho. Em kernels Linux modernos, a compilação JIT é habilitada por padrão para programas eBPF, tornando a interpretação um modo de fallback raramente usado em produção (GREGG, 2019).

## **3.5 CO-RE (Compile Once, Run Everywhere) e o formato BTF**

Um programa eBPF escrito em C acessa frequentemente estruturas internas do kernel, como informações sobre processos, conexões de rede ou descritores de arquivo. O problema é que essas estruturas podem variar entre versões do kernel: um campo que existe no offset 40 em uma versão pode estar no offset 48 em outra, ou ter sido renomeado, reorganizado ou removido. Durante anos, esse foi um dos maiores obstáculos práticos do eBPF: um programa compilado para uma versão específica do kernel simplesmente não funcionava em outra, tornando a distribuição de ferramentas eBPF um pesadelo de compatibilidade (NAKRYIKO, 2020).

A solução tradicional adotada pelo BCC (BPF Compiler Collection) era carregar o compilador LLVM e os cabeçalhos completos do kernel em cada máquina onde o programa seria executado, para recompilar o bytecode localmente em tempo de execução. Funcionava, mas com um custo alto: a instalação de dependências pesadas em cada host, tempo de compilação a cada carregamento e a necessidade de manter os cabeçalhos do kernel sincronizados.   
O CO-RE (Compile Once, Run Everywhere) resolve esse problema de forma elegante, em dois componentes que trabalham juntos. O primeiro é o BTF (BPF Type Format), um formato compacto de metadados de tipos que o kernel Linux moderno embute em si mesmo – disponível via /sys/kernel/btf/vmlinux. Ele descreve todas as estruturas internas do kernel: seus campos, tipos, tamanhos e offsets. Em vez de depender de cabeçalhos de código-fonte, o programa eBPF pode consultar o BTF do kernel em execução para descobrir onde exatamente cada campo está localizado naquela versão específica (NAKRYIKO, 2020).  
O segundo é a realocação em tempo de carregamento: quando o programa eBPF é compilado, o compilador emite anotações indicando quais acessos a campos de estruturas precisam ser ajustados. No momento do carregamento, a biblioteca libbpf lê o BTF do kernel local e ajusta automaticamente esses offsets para os valores corretos daquela versão. O bytecode compilado uma única vez se adapta ao kernel onde está sendo carregado, sem recompilação (NAKRYIKO, 2020).  
Uma analogia: imagine um manual de instruções escrito com marcadores de posição, "parafuso na posição X". Antes do CO-RE, você precisava reescrever o manual inteiro para cada modelo de máquina. Com o CO-RE, o manual tem anotações genéricas, e uma ferramenta inteligente preenche os valores corretos na hora da instalação, consultando as especificações do modelo em mãos.  
O impacto prático é significativo, as ferramentas eBPF modernas podem ser distribuídas como binários pré-compilados da mesma forma que qualquer executável Linux e funcionam em kernels diferentes sem nenhuma dependência de compilador ou cabeçalhos. Isso foi fundamental para a adoção do eBPF em ambientes de produção, onde instalar ferramentas de compilação em servidores de produção é frequentemente inviável ou indesejável (NAKRYIKO, 2020; GREGG, 2019).

## **3.6 libbpf: o papel da biblioteca no ciclo de vida de um programa eBPF**

Pense na libbpf como o gerente de obras de um projeto de construção. O bytecode eBPF é o projeto arquitetônico; o kernel é o terreno onde a construção acontece. A libbpf é quem pega o projeto, verifica se tudo está em ordem, coordena as etapas de instalação e garante que a estrutura final funcione corretamente, sem que o programador precise lidar diretamente com cada detalhe de baixo nível (Linux Kernel Documentation, 2024). Em termos técnicos, a libbpf é responsável por quatro etapas principais no ciclo de vida de um programa eBPF.   
A primeira é o carregamento do objeto: a libbpf lê o arquivo .o gerado pelo compilador, que contém o bytecode eBPF e as anotações BTF, e o prepara para ser enviado ao kernel. Nessa etapa, ela aplica as realocações CO-RE, ajustando os offsets de acesso a estruturas do kernel de acordo com o BTF do sistema local (NAKRYIKO, 2020).  
A segunda é a submissão ao kernel via syscall: a libbpf usa a syscall bpf() para submeter o bytecode ao kernel. É nesse momento que o verificador entra em ação, analisando o programa. Se aprovado, o kernel retorna um file descriptor, um identificador numérico que representa o programa carregado, que a libbpf mantém para uso posterior (Linux Kernel Documentation, 2024).  
A terceira é o anexo ao ponto de ancoragem (hook): um programa eBPF carregado ainda não faz nada, ele precisa ser anexado a um ponto de observação no sistema para começar a ser executado. A libbpf fornece funções de alto nível para anexar programas a kprobes, tracepoints, interfaces de rede (XDP, TC) e outros hooks, abstraindo as chamadas de sistema específicas de cada tipo (GREGG, 2019).  
A quarta é o gerenciamento de mapas: a libbpf cria e gerencia os mapas eBPF, as estruturas de dados compartilhadas entre o kernel e o espaço do usuário, e expõe funções para leitura, escrita e iteração sobre seus conteúdos a partir do programa em espaço do usuário (GREGG, 2019; NAKRYIKO, 2020).  
Ao encerrar o programa, a libbpf também cuida da desanexação dos hooks e da liberação dos recursos alocados. Esse ciclo completo é o fluxo que todo programa eBPF segue, e a libbpf é a camada que torna esse fluxo acessível sem exigir que o desenvolvedor manipule diretamente as syscalls do kernel.

# **4\. Pontos de ancoragem (hooks): onde o eBPF se conecta ao sistema**

Um hook é como uma tomada elétrica estrategicamente posicionada dentro do kernel, o programa eBPF é o aparelho que você conecta. Dependendo de qual tomada você usa, você tem acesso a tipos diferentes de eventos. A escolha do hook certo para cada finalidade é uma das decisões mais importantes no desenvolvimento com eBPF. Os hooks não são excludentes, um sistema pode ter múltiplos programas eBPF ativos simultaneamente, cada um conectado a um hook diferente, cada um com sua finalidade específica.

## **4.1 kprobes: instrumentação de funções do kernel**

	As kprobes são o mecanismo mais poderoso e flexível de instrumentação disponível no eBPF. Elas permitem anexar um programa eBPF a praticamente qualquer função interna do kernel sem modificar o código fonte.  
	Para entender o que isso significa na prática, pense no kernel como uma fábrica com centenas de estações de trabalho, cada uma responsável por uma operação específica: uma estação processa os pacotes TCP, outra aloca a memória, outra agenda processos no processador. Um kprobe é como instalar uma câmera de monitoramento em uma estação específica: quando o operador (kernel) passa por ele para executar seu trabalho, a câmera registra o que está acontecendo sem interromper nem alterar o trabalho em si (GREGG, 2019).  
	Tecnicamente falando, quando uma kprobe é registrada em uma função, o kernel substitui temporariamente a primeira instrução dessa função por uma instrução de interrupção (breakpoint). Quando o kernel tenta executar essa função, a interrupção é disparada, o controle é transferido para o handler da kprobe e então a execução da função original é retomada normalmente. Todo esse processo ocorre em nanossegundos e é transparente para a aplicação que originou a chamada (Linux Kernel Documentation, 2024).  
	No contexto da Tese de Conclusão de Curso (TCC) que está servindo de base para este mini curso, as kprobes foram usadas exatamente dessa forma: dois programas eBPF foram anexados às funções tcp\_sendmsg e tcp\_cleanup\_rbuf do kernel – funções invocadas a cada envio e recepção de dados TCP. A cada chamada, os programas acumulam contadores de bytes transmitidos e recebidos por porta diretamente em mapas eBPF no kernel. O resultado é que o observador obtém métricas de tráfego com granularidade de conexão, sem copiar pacotes para o espaço do usuário e sem pressão adicional sobre a pilha de rede (CASSAGNES et al., 2020).  
	Essa abordagem contrasta diretamente com as ferramentas baseadas em /proc: enquanto o Sysstat e o Prometheus precisam ler e processar arquivos periodicamente para estimar o tráfego, o coletor eBPF com kprobes contabiliza cada byte no exato momento em que ele é transmitido ou recebido, entregando ao espaço do usuário apenas os totais já agregados quando solicitado.

## **4.2 uprobes: instrumentação de funções em espaço de usuário**

	Se as kprobes instrumentam funções dentro do kernel, as uprobes fazem o equivalente no espaço do usuário: permitem anexar um programa eBPF a qualquer função de uma biblioteca ou executável, interceptando sua execução no momento da entrada ou do retorno.  
A analogia da fábrica se estende naturalmente: se as kprobes são câmeras instaladas nas estações de trabalho internas da fábrica, as uprobes são câmeras instaladas nos escritórios dos fornecedores e clientes (no espaço externo onde as aplicações operam). Elas permitem observar o comportamento de programas que rodam fora do kernel sem precisar alterá-los.  
Tecnicamente, o mecanismo é similar ao das kprobes: o kernel identifica o endereço da função alvo no binário do processo e insere um ponto de interrupção nesse endereço. Quando o processo atinge essa instrução durante sua execução, o controle é transferido para o handler da uprobe e então a execução do processo continua normalmente (Linux Kernel Documentation, 2024).  
Um caso clássico é a instrumentação de bibliotecas de linguagens de alto nível sem suporte nativo a rastreamento: é possível, por exemplo, anexar uprobes às funções internas do interpretador Python para medir o tempo de execução de funções específicas de uma aplicação, identificar quais módulos estão sendo chamados com mais frequência, ou rastrear alocações de memória.  
No contexto de monitoramento de VNFs, as uprobes são particularmente úteis quando a função de rede é implementada como um processo em espaço do usuário e há interesse em instrumentar funções internas da aplicação que não são visíveis pelo kernel. Por exemplo, seria possível usar uma uprobe para medir o tempo de execução da função de inspeção de padrões maliciosos dentro do WAF, com granularidade de microssegundos, sem alterar o código da aplicação (CASSAGNES et al., 2020).  
Uma limitação importante das uprobes em relação às kprobes é o custo de instrumentação: como operam no espaço do usuário, cada ativação de uma uprobe requer uma transição entre espaço do usuário e kernel para executar o handler eBPF, o que introduz overhead maior do que kprobes em funções de kernel muito frequentes. Para funções chamadas milhões de vezes por segundo, esse custo pode se tornar perceptível e deve ser considerado na escolha do hook adequado (GREGG, 2019).

## **4.3 tracepoints: pontos estáticos de rastreamento**

Se as kprobes são câmeras instaladas dinamicamente em qualquer lugar da fábrica, os tracepoints são tomadas de energia já embutidas na planta original da construção.  
Em termos técnicos, tracepoints são pontos de instrumentação estáticos inseridos diretamente no código-fonte do kernel pelos seus mantenedores. Eles existem em locais considerados estáveis e relevantes para observabilidade. A lista completa de tracepoints disponíveis em um sistema pode ser consultada em /sys/kernel/debug/tracing/events (Linux Kernel Documentation, 2024).  
A diferença fundamental em relação às kprobes é a estabilidade da interface: enquanto os nomes e assinaturas das funções internas do kernel podem mudar entre versões, os tracepoints são tratados como parte da Application Binary Interface (ABI) do kernel. Os mantenedores se comprometem a não removê-los ou modificá-los sem aviso prévio. Isso os torna a escolha preferida quando se quer construir ferramentas de monitoramento portáveis e duráveis, que funcionem em múltiplas versões do kernel sem manutenção constante (GREGG, 2019).  
Do ponto de vista do custo, tracepoints têm overhead praticamente zero quando não há nenhum programa eBPF anexado a eles – eles existem no código como instruções de salto condicional que são ignoradas durante a execução normal. Quando um programa eBPF é anexado, o kernel ativa o tracepoint e passa a executar o handler a cada ocorrência do evento. Ao desanexar o programa, o tracepoint volta ao estado inativo sem deixar rastros (Linux Kernel Documentation, 2024).  
Uma analogia: pense nos tracepoints como sinalizadores de emergência já instalados em pontos estratégicos de uma rodovia (nas curvas perigosas, nas interseções, nas subidas). Eles estão lá, fixos e padronizados, esperando. Quando você precisa monitorar um trecho específico, basta ativar os sinalizadores daquele ponto; quando terminar, desativa e eles somem da paisagem sem deixar obra inacabada.  
No contexto de monitoramento de VNFs, tracepoints são especialmente úteis para observar eventos de ciclo de vida de processos e eventos da pilha de rede em pontos bem definidos, como net:netif\_receive\_skb (recepção de pacote na interface) ou sock:inet\_sock\_set\_state (mudança de estado de uma conexão TCP). Esses eventos fornecem uma visão de alto nível do comportamento da rede com overhead mínimo e interface estável entre versões de kernel (GREGG, 2019; CASSAGNES et al., 2020).

## **4.4 Traffic Control (TC): inspeção de pacotes na pilha de rede**

O Traffic Control (TC) é o subsistema do kernel Linux responsável por gerenciar como os pacotes são enfileirados, priorizados e encaminhados nas interfaces de rede. Com o eBPF, ele se torna também um poderoso ponto de inspeção e modificação de pacotes — posicionado em uma camada mais profunda da pilha de rede do que as ferramentas tradicionais, mas com acesso a mais contexto do que o XDP(GREGG, 2019).  
Retomando a analogia da fábrica: se o kernel é a fábrica e os pacotes de rede são produtos sendo transportados por esteiras, o TC é o sistema de triagem e controle de fluxo dessas esteiras. Um programa eBPF anexado ao TC se posiciona exatamente nesse ponto de triagem.  
Tecnicamente, programas eBPF podem ser associados ao TC nos dois sentidos do tráfego de cada interface: no ponto de entrada (ingress), onde os pacotes chegam à interface antes de serem processados pelo kernel, e no ponto de saída (egress), onde os pacotes estão prestes a ser transmitidos. Em ambos os casos, o programa tem acesso à estrutura sk\_buff (a representação interna do pacote no kernel), que contém os cabeçalhos completos de todas as camadas do protocolo (Ethernet, IP, TCP/UDP) e metadados associados à conexão (Linux Kernel Documentation, 2024).  
Esse acesso ao sk\_buff é o que diferencia o TC do XDP: enquanto o XDP opera antes da alocação dessa estrutura e portanto tem acesso apenas aos bytes brutos do pacote, o TC tem visibilidade sobre o estado completo da conexão mantido pelo kernel — incluindo informações como o estado da máquina de estados TCP, o socket associado e metadados de Quality of Service (QoS). Essa riqueza de contexto torna o TC ideal para casos de uso que exigem decisões baseadas no estado da conexão, não apenas no conteúdo do pacote (GREGG, 2019; CASSAGNES et al., 2020).  
Na prática, o TC é amplamente usado em soluções de rede para contêineres. O Cilium, por exemplo, utiliza programas eBPF no TC para implementar políticas de segurança entre pods Kubernetes. Para monitoramento de VNFs, o TC oferece um ponto privilegiado para medir vazão e latência por fluxo em interfaces virtuais, com sobrecarga inferior à das ferramentas baseadas em /proc e granularidade superior à de contadores agregados (CASSAGNES et al., 2020).

## **4.5 XDP: processamento de pacotes de altíssima velocidade**

O eXpress Data Path (XDP)  é o hook mais próximo do hardware disponível no eBPF. Ele permite que programas eBPF processem pacotes no momento em que eles chegam ao driver da placa de rede, antes mesmo que o kernel aloque qualquer estrutura de memória para representá-los. É, em essência, o ponto mais precoce possível de intervenção na jornada de um pacote dentro do sistema (GREGG, 2019).  
A analogia mais precisa para o XDP é a de uma cabine de pedágio na entrada de uma rodovia: antes de o veículo (o pacote) entrar na rodovia propriamente dita (a pilha de rede do kernel), ele passa pela cabine, que decide instantaneamente o que fazer. Essa decisão é tomada antes que qualquer recurso da rodovia seja consumido. Se o veículo for barrado, ele nunca chegou a ocupar espaço na pista — e isso é exatamente o que torna o XDP tão eficiente para casos como mitigação de ataques DDoS: pacotes maliciosos são descartados antes de consumir memória, CPU de processamento de pilha ou largura de banda interna do sistema (GREGG, 2019; CASSAGNES et al., 2020).  
Tecnicamente, um programa XDP recebe como entrada os bytes brutos do pacote e deve retornar uma das seguintes ações: XDP\_PASS (encaminhar o pacote para o processamento normal do kernel), XDP\_DROP (descartar o pacote imediatamente), XDP\_TX (retransmitir o pacote pela mesma interface, útil para implementar roteadores simétricos), ou XDP\_REDIRECT (encaminhar o pacote para outra interface ou para um socket de espaço do usuário via AF\_XDP) (Linux Kernel Documentation, 2024).  
O desempenho é notável: implementações XDP em hardware comum atingem taxas de processamento da ordem de dezenas de milhões de pacotes por segundo por núcleo de CPU — valores que ferramentas de espaço do usuário, mesmo otimizadas como o DPDK, só conseguem com técnicas de busy polling que consomem um núcleo inteiro de forma exclusiva. O XDP alcança isso sem monopolizar o processador, pois opera no contexto normal de interrupção do driver (GREGG, 2019).  
A principal limitação do XDP, como mencionado, é justamente sua posição tão precoce na pilha: por ser executado antes do sk\_buff, o programa não tem acesso a informações de estado de conexão mantidas pelo kernel. Para monitoramento de métricas orientadas a fluxo, o TC ou as kprobes são escolhas mais adequadas. O XDP brilha quando o objetivo é tomar decisões rápidas baseadas exclusivamente nos cabeçalhos do pacote, sem necessidade de contexto de conexão (GREGG, 2019; CASSAGNES et al., 2020).

## **4.6 Mapas eBPF (BPF Maps): comunicação entre kernel e espaço de usuário**

Os mapas eBPF são o elo que conecta tudo que foi apresentado até aqui. Sem eles, um programa eBPF seria capaz de observar eventos dentro do kernel, mas incapaz de comunicar o que observou para o mundo externo.  
Os mapas eBPF são estruturas de dados chave-valor residentes no kernel, acessíveis de dois lados: pelo programa eBPF em execução no kernel, e pelo programa de controle no espaço do usuário. Essa característica de acesso compartilhado é o que os torna o mecanismo central de comunicação assíncrona entre os dois espaços (GREGG, 2019).  
O programa eBPF atualiza os mapas no exato momento em que os eventos ocorrem, sem precisar interromper sua execução para notificar ninguém. O programa no espaço do usuário consulta os mapas quando quiser, lendo os valores agregados que o programa eBPF foi acumulando ao longo do tempo. Não há polling de arquivos /proc, não há serialização de dados pela rede, não há cópias desnecessárias, apenas uma leitura direta de uma estrutura de dados em memória (GREGG, 2019; NAKRYIKO, 2020).  
No experimento do TCC base deste curso, os mapas foram usados exatamente dessa forma: os programas eBPF anexados a tcp\_sendmsg e tcp\_cleanup\_rbuf acumulavam contadores de bytes por porta em mapas do tipo HASH. Quando o observador recebia uma requisição UDP do probe.py, ele simplesmente lia esses contadores do mapa e os retornava (CASSAGNES et al., 2020).  
O eBPF oferece diversos tipos de mapas, cada um otimizado para um padrão de acesso diferente. O tipo HASH armazena pares chave-valor com acesso por chave arbitrária. O tipo ARRAY armazena valores em posições numéricas fixas, útil para histogramas e contadores globais. O tipo PERF\_EVENT\_ARRAY e o RINGBUF permitem que o programa eBPF envie eventos individuais para o espaço do usuário em tempo real, como logs de eventos ou amostras de latência. O tipo LRU\_HASH implementa um hash com política de substituição dos itens menos recentemente usados, útil para rastrear conexões ativas sem crescimento ilimitado de memória (Linux Kernel Documentation, 2024; GREGG, 2019).  
Uma analogia para os mapas: pense neles como um quadro de avisos compartilhado entre a cozinha (kernel) e o salão (espaço do usuário) do restaurante da nossa analogia original. O cozinheiro (programa eBPF) atualiza o quadro a cada prato preparado. O gerente (programa no espaço do usuário) consulta o quadro quando precisa tomar uma decisão, sem precisar entrar na cozinha, sem interromper o cozinheiro e sem esperar que alguém venha entregar o relatório.

# **5\. eBPF vs. abordagens tradicionais de monitoramento**

Ao longo dos capítulos anteriores, construímos uma compreensão sólida de como o eBPF funciona por dentro. Agora é o momento de colocá-lo lado a lado com as ferramentas que dominaram o monitoramento de sistemas Linux por décadas e entender, de forma concreta, o que muda na prática quando se escolhe uma abordagem ou outra.

## **5.1 Monitoramento via /proc**

O /proc é um pseudossistema de arquivos mantido pelo kernel Linux que expõe o estado interno do sistema como uma hierarquia de arquivos de texto legíveis no espaço do usuário. Ele não ocupa espaço em disco, seus "arquivos" são gerados dinamicamente pelo kernel a cada leitura. Informações como uso de CPU por processo, contadores de bytes por interface de rede, estatísticas de memória e estado de conexões TCP estão todas acessíveis ali, organizadas em caminhos como /proc/stat, /proc/net/dev e /proc/\<pid\>/stat (Linux man-pages project, 2024).  
A elegância do /proc está na sua simplicidade: qualquer programa que saiba ler um arquivo de texto pode coletar métricas do sistema, sem privilégios especiais, sem dependências externas e sem modificar nada.   
O problema aparece quando a frequência de coleta aumenta ou quando o ambiente monitorado passa a operar sob alta carga. Cada leitura de um arquivo /proc é, na prática, uma operação sincronizada com o kernel: o processo leitor precisa cruzar a fronteira entre espaço do usuário e espaço do kernel, o kernel serializa os contadores internos para texto, e o processo leitor os recebe, faz o parsing e calcula os deltas. Esse ciclo tem um custo que, repetido centenas de vezes por segundo em ambientes com muitas interfaces ou processos monitorados, começa a competir pelos mesmos ciclos de CPU que deveriam estar disponíveis para as aplicações monitoradas (Linux man-pages project, 2024; CASSAGNES et al., 2020).  
Há também uma limitação estrutural de granularidade: os contadores do /proc são agregados, eles mostram totais acumulados desde o início do sistema ou médias sobre o intervalo de amostragem. Um pico de latência que dura 200 milissegundos e se resolve antes da próxima amostra de um segundo simplesmente desaparece nos dados. Para sistemas onde eventos transitórios de curta duração são sintomas importantes de problemas em desenvolvimento, essa cegueira temporal é uma limitação real (RESEARCH, 2024).  
Uma analogia: o /proc é como um painel de instrumentos de um carro antigo — ele mostra a velocidade atual e o nível de combustível, mas não registra quantas vezes o motor bateu em um pico de temperatura nos últimos cinco minutos, nem qual foi a aceleração máxima em cada curva. É informação suficiente para dirigir com segurança no dia a dia, mas insuficiente para diagnosticar um problema intermitente que só aparece sob condições específicas.

## **5.2 Ferramentas clássicas: Sysstat e Prometheus**

O monitoramento de sistemas operacionais sempre foi uma necessidade fundamental para administradores de infraestrutura.

### **5.2.1 Sysstat**

O Sysstat é um conjunto de utilitários consolidado ao longo de décadas de uso em sistemas Linux. Ferramentas como sar, iostat e pidstat coletam métricas de desempenho lendo periodicamente os arquivos do /proc e calculando deltas entre amostras consecutivas para estimar taxas de atividade — CPU por processo, bytes por interface de rede, operações de I/O por disco, entre outras (GODARD; Sysstat Contributors, 2024).  
Sua principal vantagem é a maturidade e a ubiquidade: está disponível na maioria das distribuições Linux, tem documentação extensa e é amplamente conhecido por administradores de sistemas. Para monitoramento de linha de base em ambientes de baixa a média carga, cumpre bem o papel com configuração mínima.  
A limitação central é o modelo de polling periódico. O Sysstat lê o /proc em ciclos fixos, normalmente a cada segundo ou mais. Isso significa que qualquer evento que ocorra e se resolva entre dois ciclos de leitura é invisível para a ferramenta. Além disso, sob intervalos de amostragem inferiores a um segundo, a sobrecarga do polling começa a ser perceptível: o processo leitor consome ciclos de CPU que concorrem diretamente com as aplicações monitoradas, especialmente em ambientes com muitos processos ou interfaces de rede (GODARD; Sysstat Contributors, 2024; CASSAGNES et al., 2020).  
No experimento do TCC base deste curso, o coletor Sysstat apresentou tempo de resposta médio entre 0,57 ms e 0,60 ms nos quatro volumes de carga testados — consistentemente superior ao do coletor eBPF em todas as condições, com a diferença sendo estatisticamente significativa ao nível de confiança de 95%.

### **5.2.2 Prometheus**

O Prometheus consolidou-se como o padrão de observabilidade em arquiteturas de microsserviços e ambientes baseados em contêineres. Seu modelo de coleta é baseado em pull: um servidor central realiza requisições HTTP periódicas a exporters, agentes que expõem métricas em um endpoint /metrics no formato de texto padronizado pelo Prometheus (TURNBULL, 2018).  
A grande força do Prometheus é o ecossistema: há exporters para praticamente qualquer tecnologia (bancos de dados, servidores web, sistemas operacionais, aplicações customizadas) e uma linguagem de consulta expressiva (PromQL) que permite construir alertas e dashboards sofisticados. Para ambientes Kubernetes, é frequentemente o primeiro passo em qualquer estratégia de observabilidade.  
Do ponto de vista de overhead, o Prometheus herda os custos do /proc (via exporters como o Node Exporter) e adiciona camadas extras: o exporter mantém um servidor HTTP em execução constante, serializa as métricas em memória a cada atualização interna e as disponibiliza via rede para o servidor Prometheus. Em ambientes onde o exporter está co-localizado com a VNF monitorada, o servidor HTTP e a serialização contínua consomem recursos adicionais que competem com a função de rede (TURNBULL, 2018; CASSAGNES et al., 2020).  
No experimento do TCC, o coletor Prometheus apresentou o maior consumo de memória entre as três abordagens (aproximadamente 78% acima do Sysstat e 60% acima do eBPF), decorrente diretamente da biblioteca prometheus\_client e do servidor HTTP mantido em execução. O tempo de resposta também foi consistentemente o mais alto em três dos quatro volumes testados, compatível com o custo adicional de manter o servidor HTTP ativo.

## **5.3 Vantagens e limitações do eBPF frente a essas abordagens**

Com as abordagens tradicionais contextualizadas, é possível traçar uma comparação direta e honesta.

### **5.3.1 Vantagens do eBPF**

A primeira e mais fundamental vantagem é a eliminação do polling. O eBPF não consulta o sistema periodicamente, ele reage a eventos no exato momento em que acontecem. Um programa eBPF anexado a tcp\_sendmsg é executado a cada byte transmitido, acumulando o contador no mapa diretamente no kernel. Quando o espaço do usuário solicita o valor, ele lê um contador já consolidado — sem varredura de arquivos, sem parsing de texto, sem alternância de contexto adicional (GREGG, 2019; CASSAGNES et al., 2020).  
A segunda vantagem é a granularidade temporal. Como o eBPF captura cada evento individualmente, ele pode detectar anomalias de curta duração que o polling jamais veria, um pico de latência de 50 ms em uma conexão TCP específica, uma rajada de tráfego que dura menos de um segundo, uma alocação de memória incomum que ocorre e se resolve entre dois ciclos de amostragem (RESEARCH, 2024).  
A terceira é a especificidade de escopo. Enquanto ferramentas baseadas em /proc/net/dev coletam contadores de todas as interfaces de rede do host, incluindo interfaces virtuais de outros contêineres ou VMs co-localizados, o eBPF pode filtrar no nível do kernel, contabilizando apenas o tráfego relevante para a VNF de interesse.  
A quarta vantagem é o menor overhead de CPU do coletor. Em N \= 100.000 mensagens, o coletor eBPF consumiu em média apenas 0,05% de CPU, valor uma a duas ordens de grandeza inferior ao dos coletores em espaço de usuário (4,66% no Sysstat e 2,37% no Prometheus). Essa diferença é especialmente relevante em ambientes de borda com recursos computacionais escassos, onde cada fração de CPU desviada para monitoramento representa capacidade de inspeção subtraída da VNF (CASSAGNES et al., 2020).

### **5.3.2 Limitações do eBPF**

A primeira limitação é a complexidade de desenvolvimento. Escrever um programa eBPF em C, lidar com o verificador e gerenciar o ciclo de vida via libbpf exige um nível de conhecimento técnico significativamente maior do que configurar o Node Exporter do Prometheus ou usar o sar do Sysstat. O verificador em particular pode gerar mensagens de erro difíceis de interpretar para quem está começando, tornando a curva de aprendizado mais íngreme (GREGG, 2019).  
A segunda é o requisito de kernel moderno. As funcionalidades mais avançadas do eBPF requerem kernels Linux 5.0 ou superiores, preferencialmente 5.8 ou mais recentes. Em ambientes com kernels legados, o eBPF pode não estar disponível ou ter funcionalidades limitadas (Linux Kernel Documentation, 2024).  
A terceira é a variabilidade entre execuções. No experimento do TCC, o coletor eBPF apresentou maior dispersão entre execuções em comparação com os coletores em espaço de usuário, que exibiram distribuições mais concentradas. Isso é coerente com a natureza reativa do eBPF: sob variações de carga, o tempo de resposta do coletor pode oscilar mais, ainda que seus piores casos típicos permaneçam competitivos com a mediana das ferramentas tradicionais (CASSAGNES et al., 2020).  
A quarta é o risco de degradação por uso inadequado. Como discutido na seção 2.3, programas eBPF mal dimensionados podem degradar o desempenho de todas as aplicações no mesmo host devido à violação do isolamento de desempenho. Um programa que instrumenta funções chamadas milhões de vezes por segundo com lógica complexa pode introduzir latência perceptível no caminho crítico do kernel.

## **5.4 Quando usar eBPF: riscos e trade-offs**

O eBPF é particularmente adequado para os seguintes cenários:  
**Monitoramento de alta frequência com granularidade fina:** Quando a aplicação monitorada opera em escalas de milissegundos ou microssegundos, e eventos transitórios de curta duração são sintomas importantes de problemas em desenvolvimento, o eBPF oferece uma visibilidade que o polling periódico simplesmente não consegue fornecer. Um pico de latência que dura 50 ms e se resolve antes da próxima amostra de 1 segundo é invisível para o Prometheus, mas capturável pelo eBPF (RESEARCH, 2024).  
**Ambientes com recursos computacionais restritos:** Em dispositivos de borda (edge computing), gateways IoT ou VNFs executando em hardware limitado, cada fração de CPU desviada para monitoramento representa capacidade subtraída da função principal. O consumo de CPU do coletor eBPF no experimento foi de apenas 0,05% em média, valor duas ordens de grandeza inferior ao Sysstat (4,66%) e ao Prometheus (2,37%) (CASSAGNES et al., 2020).  
**Observabilidade em ambientes conteinerizados dinâmicos:** Em clusters Kubernetes com pods que sobem e descem em segundos, agentes baseados em arquivos /proc frequentemente perdem o rastreamento de processos que são efêmeros. O eBPF, por operar no nível do kernel, mantém visibilidade sobre todos os processos independentemente de seu ciclo de vida (GREGG, 2019).  
**Instrumentação personalizada sem modificação de código:** Quando você precisa monitorar uma função específica de uma aplicação de terceiros ou de um componente do kernel sem acesso ao código-fonte ou sem permissão para recompilá-lo, as kprobes e uprobes do eBPF oferecem uma capacidade de instrumentação dinâmica que nenhuma ferramenta baseada em /proc pode fornecer.  
**Segurança e detecção de anomalias em tempo real:** Aplicações de segurança que precisam interceptar chamadas de sistema, detectar escalonamento de privilégios ou bloquear conexões maliciosas no exato momento em que ocorrem encontram no eBPF uma plataforma natural (Linux Kernel Documentation, 2024).  
Apesar de suas vantagens, o eBPF não é isento de custos e riscos que precisam ser considerados:  
**Curva de aprendizado íngreme:** Escrever um programa eBPF em C, lidar com as restrições do verificador e gerenciar o ciclo de vida via libbpf exige conhecimento técnico especializado. O verificador, em particular, rejeita padrões de código que seriam perfeitamente válidos em C comum, e suas mensagens de erro podem ser enigmáticas para quem está começando. Isso representa um custo de treinamento e desenvolvimento que não deve ser subestimado (GREGG, 2019).  
**Dependência de versões recentes do kernel:** As funcionalidades mais avançadas do eBPF requerem kernels Linux 5.0 ou superiores, preferencialmente 5.8 ou mais recentes. Em ambientes empresariais que ainda utilizam distribuições com kernels LTS mais antigos (como RHEL 7 com kernel 3.10), o eBPF pode ter funcionalidades severamente limitadas (Linux Kernel Documentation, 2024).  
**Risco de degradação de desempenho por uso inadequado:** Programas eBPF mal projetados podem degradar o desempenho de todas as aplicações no mesmo host. Um programa que instrumenta uma função chamada milhões de vezes por segundo com lógica de agregação complexa pode introduzir latência perceptível no caminho crítico do kernel. O verificador garante segurança funcional (sem loops infinitos, sem acessos de memória inválidos), mas não garante eficiência.  
**Maior variabilidade entre execuções:** Como discutido na seção anterior, o coletor eBPF apresentou maior dispersão no tempo de resposta em comparação com as abordagens tradicionais. Isso ocorre porque sua natureza reativa o torna mais sensível a variações na carga do sistema. Em cenários onde a estabilidade de resposta é mais importante do que latência mínima, essa característica deve ser considerada (CASSAGNES et al., 2020).  
**Menor maturidade do ecossistema de ferramentas:** Embora esteja crescendo rapidamente, o ecossistema de ferramentas eBPF ainda é menos maduro que o de soluções baseadas em /proc. A quantidade de bibliotecas, dashboards pré-construídos e integrações com sistemas de alerta disponíveis para Prometheus, por exemplo, é atualmente muito superior à disponível para soluções nativas eBPF.

# **6\. Preparando o ambiente prático**

Com a base teórica estabelecida, esta seção descreve o ambiente utilizado na demonstração prática do minicurso, replicando as condições do experimento conduzido no Trabalho de Conclusão de Curso que fundamenta esta proposta de extensão: uma Função de Rede Virtualizada (um Web Application Firewall) executando em contêiner Docker, observada por programas eBPF anexados a funções do kernel responsáveis pelo envio e recebimento de dados TCP.

## **6.1 Requisitos de sistema** 

	Programas eBPF modernos, que utilizam o modelo CO-RE descrito na seção 3.5, dependem de um kernel Linux relativamente recente com suporte a BTF (BPF Type Format) habilitado. Na prática, isso significa:

* Kernel Linux 5.4 ou superior, com CONFIG\_DEBUG\_INFO\_BTF=y habilitado (a maioria das distribuições Ubuntu 20.04+ já traz essa configuração por padrão);  
* Presença do arquivo /sys/kernel/btf/vmlinux, que pode ser verificada com o comando ls sobre esse caminho;  
* Permissões de superusuário (root ou capacidade CAP\_BPF/CAP\_SYS\_ADMIN) para carregar programas eBPF no kernel;  
* Docker instalado, para reproduzir o cenário de VNF em contêiner utilizado no experimento original.

## **6.2 Instalando dependências** 

	O ambiente de desenvolvimento utilizado no minicurso foi padronizado em Ubuntu, com as seguintes dependências instaladas via apt:  
sudo apt update  
sudo apt install \-y clang llvm libbpf-dev libelf-dev \\  
                     linux-tools-$(uname \-r) linux-headers-$(uname \-r) \\  
                     python3 python3-pip  
	O pacote linux-tools traz o utilitário bpftool, usado para inspecionar programas e mapas eBPF carregados no kernel, além de gerar o cabeçalho vmlinux.h a partir do BTF do kernel local. O clang/llvm compila o código C para bytecode eBPF, e o libbpf-dev fornece os cabeçalhos e a biblioteca estática usados para carregar e gerenciar os programas a partir do espaço do usuário.  
	Para a coleta e visualização das métricas, foram usadas bibliotecas Python de análise estatística e geração de gráficos, além de um script de sondagem (probe.py) responsável por consultar os mapas eBPF periodicamente.

## **6.3 Estrutura de um projeto eBPF** 

	Um projeto eBPF baseado em libbpf e CO-RE segue tipicamente uma estrutura de diretórios que separa claramente o código que roda dentro do kernel do código que roda no espaço do usuário:  
projeto-ebpf/  
  src/  
    monitor.bpf.c    (programa eBPF, espaço kernel)  
    monitor.c        (programa de controle, espaço usuário)  
  include/  
    vmlinux.h         (gerado via bpftool a partir do BTF local)  
  probe.py             (script de sondagem dos mapas, espaço usuário)  
  Makefile  
	A convenção de nomear o arquivo do programa eBPF com o sufixo .bpf.c não é obrigatória, mas é amplamente adotada pela comunidade para distinguir, à primeira vista, qual código é compilado para bytecode eBPF e qual é compilado como um binário nativo comum. O arquivo vmlinux.h é gerado uma única vez por máquina com o comando:  
bpftool btf dump file /sys/kernel/btf/vmlinux format c \> include/vmlinux.h  
	Esse cabeçalho substitui os cabeçalhos tradicionais do kernel (linux/kernel.h e similares), fornecendo ao compilador todas as definições de tipos internos do kernel local, e é a peça central que viabiliza a portabilidade CO-RE descrita na seção 3.5.

# **z7. Construindo um programa eBPF**

	Esta seção acompanha, passo a passo, a construção do programa eBPF demonstrado no minicurso: um coletor de bytes transmitidos e recebidos por porta TCP, equivalente ao utilizado no experimento do TCC para observar o tráfego do WAF em contêiner.

## **7.1 Anatomia de um programa em C** 

	Um programa eBPF em C é estruturalmente distinto de um programa C comum: ele não possui uma função main e não é executado como um processo independente. Em vez disso, define uma ou mais funções marcadas com a macro SEC(), que indica ao compilador e ao libbpf a qual tipo de hook aquela função deve ser anexada.  
// monitor.bpf.c  
\#include "vmlinux.h"  
\#include \<bpf/bpf\_helpers.h\>  
   
struct {  
    \_\_uint(type, BPF\_MAP\_TYPE\_HASH);  
    \_\_uint(max\_entries, 1024);  
    \_\_type(key, \_\_u16);   // porta TCP  
    \_\_type(value, \_\_u64); // bytes acumulados  
} bytes\_por\_porta SEC(".maps");  
   
SEC("kprobe/tcp\_sendmsg")  
int BPF\_KPROBE(trace\_tcp\_sendmsg, struct sock \*sk,  
               struct msghdr \*msg, size\_t size)  
{  
    \_\_u16 porta \= BPF\_CORE\_READ(sk, \_\_sk\_common.skc\_dport);  
    \_\_u64 \*total \= bpf\_map\_lookup\_elem(\&bytes\_por\_porta, \&porta);  
    \_\_u64 novo\_total \= total ? \*total \+ size : size;  
    bpf\_map\_update\_elem(\&bytes\_por\_porta, \&porta, \&novo\_total, BPF\_ANY);  
    return 0;  
}  
   
char LICENSE\[\] SEC("license") \= "GPL";  
	Três elementos merecem destaque. Primeiro, o mapa bytes\_por\_porta é declarado como uma estrutura anotada com macros \_\_uint e \_\_type, que o libbpf interpreta em tempo de compilação para gerar a definição do mapa esperada pelo kernel. Segundo, a macro BPF\_CORE\_READ substitui o acesso direto a campos de estruturas do kernel — como o campo de porta de destino de um struct sock —, permitindo que a leitura seja ajustada pelo mecanismo CO-RE para a versão de kernel em execução. Terceiro, a declaração LICENSE é obrigatória: o verificador do kernel recusa carregar programas que não declarem uma licença compatível com GPL para o uso de determinadas helper functions.

## **7.2 Compilação com clang/LLVM**

	O programa eBPF é compilado para bytecode com o clang, indicando explicitamente o alvo de compilação bpf:  
clang \-O2 \-g \-target bpf \-D\_\_TARGET\_ARCH\_x86 \\  
      \-I include \-c src/monitor.bpf.c \-o monitor.bpf.o  
	A flag \-g inclui informações de depuração (incluindo BTF) no objeto resultante, indispensáveis para que as realocações CO-RE funcionem no momento do carregamento. O resultado é um arquivo objeto ELF (monitor.bpf.o) contendo o bytecode eBPF, os mapas declarados e os metadados BTF do programa — pronto para ser carregado por um programa de controle em espaço de usuário.

## **7.3 Carregamento no kernel via libbpf**

	O programa de controle, compilado e executado como um binário nativo comum, usa a API de alto nível do libbpf para abrir, carregar e verificar o objeto eBPF:  
struct bpf\_object \*obj \= bpf\_object\_\_open\_file("monitor.bpf.o", NULL);  
bpf\_object\_\_load(obj);  
	É nesse momento que o verificador analisa o bytecode e as realocações CO-RE são resolvidas contra o BTF do kernel local. Se o programa for aprovado, ele fica residente no kernel, associado a um file descriptor, mas ainda não anexado a nenhum ponto de execução.

## **7.4 Captura de eventos em tempo real**

	O anexo ao hook é o que efetivamente ativa a coleta de dados. Para o programa do exemplo, o anexo é feito a duas kprobes: uma na entrada de tcp\_sendmsg (envio de dados) e outra em tcp\_cleanup\_rbuf (recepção de dados), replicando o par de pontos de instrumentação usado no experimento do TCC:  
struct bpf\_program \*prog\_send \=  
    bpf\_object\_\_find\_program\_by\_name(obj, "trace\_tcp\_sendmsg");  
bpf\_program\_\_attach\_kprobe(prog\_send, false, "tcp\_sendmsg");  
	A partir desse ponto, toda vez que o kernel executa tcp\_sendmsg — ou seja, a cada envio de dados por qualquer conexão TCP do sistema — o programa eBPF é acionado automaticamente, atualiza o mapa bytes\_por\_porta e devolve o controle ao kernel, sem qualquer intervenção do espaço do usuário.  
	**7.5 Leitura de mapas e extração de métricas**   
	O mapa bytes\_por\_porta, uma vez populado pelo programa eBPF, pode ser lido a qualquer momento pelo espaço do usuário. No experimento do TCC, essa leitura foi implementada em um script Python (probe.py) que, ao receber uma requisição UDP, consultava o mapa e retornava os contadores agregados:  
from bcc import BPF  \# ou pylibbpf/libbpf via ctypes  
   
for chave, valor in bytes\_por\_porta.items():  
    porta \= chave.value  
    total\_bytes \= valor.value  
    print(f"porta {porta}: {total\_bytes} bytes")  
	Essa etapa fecha o ciclo completo descrito na seção 3.6: o programa eBPF observa e agrega dados dentro do kernel, e o programa de controle os expõe ao espaço do usuário sob demanda, sem polling de arquivos e sem cópias desnecessárias de pacotes.

## **7.6 Aplicação ao contexto de containers Docker**

	Um ponto de atenção ao instrumentar aplicações que rodam em contêineres é que as kprobes usadas no exemplo operam sobre funções internas do kernel do host e o kernel é compartilhado entre o host e todos os contêineres Docker em execução sobre ele. Isso significa que, ao anexar um programa eBPF a tcp\_sendmsg, ele captura o tráfego de todas as conexões TCP do sistema, inclusive as originadas dentro dos contêineres, sem precisar de nenhuma configuração especial de rede ou acesso ao namespace de rede do contêiner.  
	Essa característica foi explorada diretamente no experimento do TCC: o WAF observado rodava isolado em um contêiner Docker, e os programas eBPF eram carregados no host, fora do contêiner, sem qualquer modificação na imagem ou no processo da aplicação monitorada uma vantagem prática relevante quando não se tem controle sobre o código da aplicação sendo observada, cenário comum em ambientes de produção com Funções de Rede Virtualizadas de terceiros.

# **8\. Encerramento**

## **8.1 Recapitulando os principais conceitos**

	Recapitulando os principais conceitos Ao longo deste material, percorreu-se o caminho do eBPF desde suas origens no BPF clássico de 1993 até sua forma moderna como motor de programabilidade genérica do kernel. Foram apresentados os componentes internos da tecnologia a máquina virtual, o conjunto de instruções, o verificador, a compilação JIT e o modelo CO-RE, os principais pontos de ancoragem disponíveis (kprobes, uprobes, tracepoints, TC e XDP) e o papel dos mapas eBPF como elo entre kernel e espaço do usuário. Na prática, todo esse ferramental foi aplicado à construção de um coletor de métricas de tráfego TCP, replicando o experimento que fundamentou o Trabalho de Conclusão de Curso do grupo extensionista.

## **8.2 Onde continuar estudando**

	Onde continuar estudando Para quem deseja aprofundar os conhecimentos apresentados neste minicurso, algumas referências e projetos são bons pontos de partida: O livro BPF Performance Tools, de Brendan Gregg (GREGG, 2019), referência central utilizada ao longo deste material; A documentação oficial do kernel Linux sobre BPF (Linux Kernel Documentation, 2024), para consulta técnica detalhada; O blog de Andrii Nakryiko (NAKRYIKO, 2020), mantenedor do ecossistema libbpf, com artigos aprofundados sobre CO-RE e desenvolvimento de programas eBPF; Projetos de código aberto como bpftrace, BCC (BPF Compiler Collection) e Cilium, mencionados ao longo deste material, cujos repositórios contêm dezenas de exemplos prontos para estudo.

## **8.3 Perguntas e discussão aberta**

	\[Espaço reservado para o momento de perguntas e discussão aberta ao final do minicurso — a preencher após a realização do evento, com as principais dúvidas e pontos levantados pelos participantes.\]

# **9\. Referências**

CASSAGNES, C. et al. The rise of ebpf for non-intrusive performance monitoring. In: **NOMS 2020 \- 2020 IEEE/IFIP Network Operations and Management Symposium**. IEEE Press, 2020\. p. 1–7. Disponível em: \<[https://doi.org/10.1109/NOMS47738.2020.9110434](https://doi.org/10.1109/NOMS47738.2020.9110434)\>. 

ETSI Industry Specification Group (ISG) NFV. **Network Functions Virtualisation (NFV); Architectural Framework**. Sophia Antipolis, 2014\. Disponível em: \<[https://www.etsi.org/deliver/etsi\_gs/NFV/001\_099/002/01.02.01\_60/gs\_NFV002v010201p.pdf](https://www.etsi.org/deliver/etsi_gs/NFV/001_099/002/01.02.01_60/gs_NFV002v010201p.pdf)\>. 

GODARD, S.; Sysstat Contributors. **sysstat: System performance monitor for Linux**. 2024\. \<[https://github.com/sysstat/sysstat](https://github.com/sysstat/sysstat)\>.   
GREGG, B. **BPF Performance Tools**. Boston: Addison-Wesley, 2019\.  
Linux Kernel Documentation. **BPF Documentation**. 2024\. \<[https://www.kernel.org/doc/html/latest/bpf/](https://www.kernel.org/doc/html/latest/bpf/)\>.   
Linux man-pages project. **proc(5) — Linux manual page**. 2024\. \<[https://man7.org/linux/man-pages/man5/proc.5.html](https://man7.org/linux/man-pages/man5/proc.5.html)\>.   
NAKRYIKO, A. **BPF CO-RE (Compile Once – Run Everywhere)**. 2020\. \<[https://nakryiko.com/posts/bpf-portability-and-co-re/](https://nakryiko.com/posts/bpf-portability-and-co-re/)\>.   
RESEARCH, D. P. **Passive TCP RTT Latency Monitoring and Aggregation in the Linux Kernel using epping and eBPF**. \[S.l.\], 2024\.  
VENÂNCIO, G. et al. Beyond vnfm: Filling the gaps of the etsi vnf manager to fully support vnf life cycle operations. **International Journal of Network Management**, v. 31, n. 5, p. None, September 2021\. Disponível em: \<[https://ideas.repec.org/a/wly/intnem/v31y2021i5ne2068.html](https://ideas.repec.org/a/wly/intnem/v31y2021i5ne2068.html)\>. 
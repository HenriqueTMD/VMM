#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

// Campos da tabela de páginas
#define PT_FIELDS 6           // 4 campos na tabela
#define PT_FRAMEID 0          // Endereço da memória física
#define PT_MAPPED 1           // Endereço presente na tabela
#define PT_DIRTY 2            // Página dirty
#define PT_REFERENCE_BIT 3    // Bit de referencia
#define PT_REFERENCE_MODE 4   // Tipo de acesso, converter para char
#define PT_AGING_COUNTER 5    // Contador para aging

// Tipos de acesso
#define READ 'r'
#define WRITE 'w'

// Define a função que simula o algoritmo da política de subst.
typedef int (*eviction_f)(int8_t**, int, int, int, int, int);

typedef struct {
    char *name;
    void *function;
} paging_policy_t;

// Codifique as reposições a partir daqui!
// Cada método abaixo retorna uma página para ser trocada. Note também
// que cada algoritmo recebe:
// - A tabela de páginas
// - O tamanho da mesma
// - A última página acessada
// - A primeira modura acessada (para fifo)
// - O número de molduras
// - Se a última instrução gerou um ciclo de clock
//
// Adicione mais parâmetros caso ache necessário

int fifo(int8_t** page_table, int num_pages, int prev_page,
         int fifo_frm, int num_frames, int clock) {
    
    int page=0;
    while (page_table[page][PT_FRAMEID] != fifo_frm){
	page++;	
    }
    return page;

}

int second_chance(int8_t** page_table, int num_pages, int prev_page,
                  int fifo_frm, int num_frames, int clock) {
    return -1;
}

int nru(int8_t** page_table, int num_pages, int prev_page,
        int fifo_frm, int num_frames, int clock) {
    return -1;
}

int aging(int8_t** page_table, int num_pages, int prev_page,
          int fifo_frm, int num_frames, int clock) {
    return -1;
}

int random_page(int8_t** page_table, int num_pages, int prev_page,
                int fifo_frm, int num_frames, int clock) {
    int page = rand() % num_pages;
    while (page_table[page][PT_MAPPED] == 0) // Encontra página mapeada
        page = rand() % num_pages;
    return page;
}

// Simulador a partir daqui

int find_next_frame(int *physical_memory, int *num_free_frames,
                    int num_frames, int *prev_free) {
    if (*num_free_frames == 0) {
        return -1;
    }

    // Procura por um frame livre de forma circula na memória.
    // Não é muito eficiente, mas fazer um hash em C seria mais custoso.
    do {
        *prev_free = (*prev_free + 1) % num_frames;
    } while (physical_memory[*prev_free] == 1);

    return *prev_free;
}

int simulate(int8_t **page_table, int num_pages, int *prev_page, int *fifo_frm,
             int *physical_memory, int *num_free_frames, int num_frames,
             int *prev_free, int virt_addr, char access_type,
             eviction_f evict, int clock) {
    //validaçâo do endereço pois o mesmo não pode ser maior que o numero de paginas nem ser um valor negativo
    if (virt_addr >= num_pages || virt_addr < 0) {
        printf("Invalid access \n");
        exit(1);
    }

    if (page_table[virt_addr][PT_MAPPED] == 1) {
        page_table[virt_addr][PT_REFERENCE_BIT] = 1;
        return 0; // Not Page Fault!
    }

    int next_frame_addr;
    if ((*num_free_frames) > 0) { // Ainda temos memória física livre!
        next_frame_addr = find_next_frame(physical_memory, num_free_frames,
                                          num_frames, prev_free);
        if (*fifo_frm == -1)
            *fifo_frm = next_frame_addr;
        *num_free_frames = *num_free_frames - 1;
    } else { // Precisamos liberar a memória!
        assert(*num_free_frames == 0);
        int to_free = evict(page_table, num_pages, *prev_page, *fifo_frm,
                            num_frames, clock);
        assert(to_free >= 0);
        assert(to_free < num_pages);
        assert(page_table[to_free][PT_MAPPED] != 0);
	
        next_frame_addr = page_table[to_free][PT_FRAMEID];
        *fifo_frm = (*fifo_frm + 1) % num_frames;
        // Libera pagina antiga
        page_table[to_free][PT_FRAMEID] = -1;
        page_table[to_free][PT_MAPPED] = 0;
        page_table[to_free][PT_DIRTY] = 0;
        page_table[to_free][PT_REFERENCE_BIT] = 0;
        page_table[to_free][PT_REFERENCE_MODE] = 0;
        page_table[to_free][PT_AGING_COUNTER] = 0;
    }

    // Coloca endereço físico na tabela de páginas!
	
    int8_t *page_table_data = page_table[virt_addr];
    page_table_data[PT_FRAMEID] = next_frame_addr;
    page_table_data[PT_MAPPED] = 1;
    if (access_type == WRITE) {
        page_table_data[PT_DIRTY] = 1;
    }
    page_table_data[PT_REFERENCE_BIT] = 1;
    page_table_data[PT_REFERENCE_MODE] = (int8_t) access_type;
    *prev_page = virt_addr;

    if (clock == 1) {
        for (int i = 0; i < num_pages; i++)
            page_table[i][PT_REFERENCE_BIT] = 0;
    }

    return 1; // Page Fault!
}

//Run le e armazena a tabela do arquivo e chama a funçâo simulate para execuçâo
void run(int8_t **page_table, int num_pages, int *prev_page, int *fifo_frm,
         int *physical_memory, int *num_free_frames, int num_frames,
         int *prev_free, eviction_f evict, int clock_freq) {
    int virt_addr;//Primeira coluna da anomaly.dat
    char access_type;//Segunda coluna da anomaly.dat
    int i = 0;
    int clock = 0;
    int faults = 0;
    while (scanf("%d", &virt_addr) == 1) {
        getchar();
        scanf("%c", &access_type);
        clock = ((i+1) % clock_freq) == 0;
        faults += simulate(page_table, num_pages, prev_page, fifo_frm,
                           physical_memory, num_free_frames, num_frames,
			   prev_free,virt_addr, access_type, evict, clock);
        i++;
    }
    printf("%d\n", faults);
}

int parse(char *opt) {
    char* remainder;
    int return_val = strtol(opt, &remainder, 10);
    if (strcmp(remainder, opt) == 0) {
        printf("Error parsing: %s\n", opt);
        exit(1);
    }
    return return_val;
}

//função que le a primeira linha da tabela anomaly.dat disponibilizada e armazena os dados nos ponteiros recebidos
void read_header(int *num_pages, int *num_frames) {
    scanf("%d %d\n", num_pages, num_frames);
}


//argv[0] == ./ + nome do arquivo gerado pelo compilador
//argv[1] == <algorithm> ou o nome da função selecionada como FIFO,Random e etc
//argv[2] == <clock_freq> frequencia do clock EX: "10 < anomaly.dat"

int main(int argc, char **argv) {
    if (argc < 3) {// Verifica se o usuario preencheu todos os campos necessarios
        printf("Usage %s <algorithm> <clock_freq>\n", argv[0]);// Caso não tenha preenchido mostra o formato valido
        exit(1);
    }

    char *algorithm = argv[1];//armazena a função digitada pelo usuario
    int clock_freq = parse(argv[2]);//chama a função para converter o valor da frequencia do clock digitada pelo usuario e armazena o valor convertido
    int num_pages;
    int num_frames;
    read_header(&num_pages, &num_frames);//funçao que pega os numeros de paginas e frames e armazena nos ponteiros
	

    // Aponta para cada função que realmente roda a política de parse
    // Cria uma lista de estruturas com as funcoes disponiveis no programa
    // policies[0]: nome da funçâo
    // policies[1]: ponteiro para chamada da funçâo
    paging_policy_t policies[] = {
            {"fifo", *fifo},
            {"second_chance", *second_chance},
            {"nru", *nru},
            {"aging", *aging},
            {"random", *random_page}
    };

    int n_policies = sizeof(policies) / sizeof(policies[0]);//calcula o numero de funções
    eviction_f evict = NULL; 
    for (int i = 0; i < n_policies; i++) {
        if (strcmp(policies[i].name, algorithm) == 0) {//Compara a função digitada pelo usuario com as funçôes disponiveis
            evict = policies[i].function;//Armazena na variavel evict a função digitada pelo usuario caso seja valida
            break;
        }
    }

    if (evict == NULL) {//Teste para verificar se a função digitada pelo usuario e valida caso não seja o programa fecha
        printf("Please pass a valid paging algorithm.\n");
        exit(1);
    }

    // Aloca tabela de páginas
    int8_t **page_table = (int8_t **) malloc(num_pages * sizeof(int8_t*));
    for (int i = 0; i < num_pages; i++) {//aloca e inicia os campos da tabela referente as paginas.
        page_table[i] = (int8_t *) malloc(PT_FIELDS * sizeof(int8_t));
        page_table[i][PT_FRAMEID] = -1;
        page_table[i][PT_MAPPED] = 0;
        page_table[i][PT_DIRTY] = 0;
        page_table[i][PT_REFERENCE_BIT] = 0;
        page_table[i][PT_REFERENCE_MODE] = 0;
        page_table[i][PT_AGING_COUNTER] = 0;
    }

    // Memória Real é apenas uma tabela de bits (na verdade uso ints) indicando
    // quais frames/molduras estão livre. 0 == livre!
    // 1 memoria fisica para cada frame.
    int *physical_memory = (int *) malloc(num_frames * sizeof(int));
    for (int i = 0; i < num_frames; i++) {//inicia todas as memoria fisicas como memorias livres
        physical_memory[i] = 0;
    }
    int num_free_frames = num_frames;
    int prev_free = -1;
    int prev_page = -1;
    int fifo_frm = -1;

    // Roda o simulador
    srand(time(NULL));
   
    run(page_table, num_pages, &prev_page, &fifo_frm, physical_memory,
        &num_free_frames, num_frames, &prev_free, evict, clock_freq);

    // Liberando os mallocs
    for (int i = 0; i < num_pages; i++) {
        free(page_table[i]);
    }
    free(page_table);
    free(physical_memory);
}

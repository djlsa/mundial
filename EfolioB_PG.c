/*
 ============================================================================
 Name        : EfolioB_PG.c
 Author      : David Salsinha
 ============================================================================
 */

//  chcp 65001

// fator de multiplicação da capacidade do vetor quando cheio
#define FATOR_EXPANSAO_VETOR 1.5
// tamanho do buffer de leitura de cada linha do ficheiro
#define TAMANHO_BUFFER_LINHA 1024
// caractere que indica o final de uma lista no ficheiro
#define CARACTERE_FIM_LISTA L'X'

#define CAPACIDADE_GRUPOS 16
#define CAPACIDADE_EQUIPAS 32
#define CAPACIDADE_JOGADORES 64
#define CAPACIDADE_JOGOS 64

#include <stdio.h>
#include <stdlib.h>

#include <stdbool.h>
#include <wchar.h>
#include <wctype.h>
#include <math.h>
#include <ctype.h>

wchar_t* _str_trim(wchar_t* string) {
	wchar_t* nova;
	wchar_t* inicio = string - 1;
	wchar_t* fim = string + wcslen(string);

	while(iswspace((*++inicio)));
	// chegou ao fim (\0), portanto a string está tecnicamente vazia
	if(*inicio == 0) {
		nova = malloc(1 * sizeof(wchar_t));
		*nova = 0;
	} else {
		while(--fim > inicio && iswspace(*fim));
		size_t tamanho = (++fim - inicio);
		nova = malloc((tamanho + 1) * sizeof(wchar_t));
		wcsncpy(nova, inicio, tamanho);
		nova[tamanho] = 0;
	}
	return nova;
}

wchar_t** _str_split(wchar_t* string, wchar_t* separadores, int n_partes) {
	if(n_partes < 1)
		return NULL;
	wchar_t** partes = malloc(n_partes * sizeof(wchar_t*));
	wchar_t* tmp = string;
	int vezes = 0;
	size_t posicao = 0;
	while(vezes < n_partes) {
		wchar_t* tmp2 = _str_trim(tmp);
		free(tmp);
		tmp = tmp2;
		if(vezes < n_partes - 1) {
			posicao = wcscspn(tmp, separadores);
			if(tmp[posicao] == 0) {
				free(tmp);
				free(partes);
				return NULL;
			}
		} else
			posicao = wcslen(tmp);
		partes[vezes] = malloc((posicao + 1) * sizeof(wchar_t));
		wcsncpy(partes[vezes], tmp, posicao);
		partes[vezes][posicao] = 0;
		tmp += posicao + 1;
		vezes++;
	}
	free(tmp);
	return partes;
}

void _free_array(wchar_t** array, int num_elementos) {
	int i;
	for(i = 0; i < num_elementos; i++)
		free(array[i]);
	free(array);
}

typedef struct _chave_valor {
	wchar_t* chave;
	void* valor;
	size_t tamanho_elemento;
} ChaveValor;

ChaveValor* ChaveValor_new(wchar_t* chave, void* valor, size_t tamanho_elemento) {
	ChaveValor* this = malloc(sizeof(ChaveValor));
	this->chave = malloc(wcslen(chave) * sizeof(wchar_t));
	wcscpy(this->chave, chave);
	this->valor = malloc(tamanho_elemento);
	this->valor = valor;
	return this;
}

int ChaveValor_compare(const void* a, const void* b) {
	ChaveValor* cv1 = *(ChaveValor**) a;
	ChaveValor* cv2 = *(ChaveValor**) b;
	return wcscmp(cv1->chave, cv2->chave);
}

int ChaveValor_compare_key(const void* chave, const void* valor) {
	wchar_t* c = *(wchar_t**) chave;
	ChaveValor* cv = *(ChaveValor**) valor;
	return wcscmp(c, cv->chave);
}

typedef struct _vetor {
	size_t tamanho_elemento;
	void** array_sequencial;
	ChaveValor** array_associativo;
	int capacidade;
	int indice;
} Vetor;

Vetor* Vetor_new(int capacidade, size_t tamanho_elemento) {
	if(capacidade < 1)
		return NULL;

	// Aloca espaço em memória para armazenar a estrutura
	Vetor* this = malloc(sizeof(Vetor));

	this->tamanho_elemento = tamanho_elemento;

	// Aloca espaço em memória para o array
	this->array_sequencial = malloc(capacidade * tamanho_elemento);
	this->array_associativo = malloc(capacidade * sizeof(ChaveValor*));

	// Inicialização de variáveis da estrutura
	this->capacidade = capacidade;
	this->indice = 0;

	return this;
}

void Vetor_free(Vetor* this) {
	// Liberta o array_sequencial
	//_free_array(this->array_sequencial, this->capacidade);
	//_free_array((void**)this->array_associativo, this->capacidade);
	free(this);
}

/*
 * Função que retorna o numero de elementos armazenados na lista
 * lista: apontador para a estrutura que representa a lista
 *
 * Devolve: numero de elementos
 */
int Vetor_length(Vetor* this) {
	return this->indice;
}

void Vetor_print(Vetor* this) {
	int i, t = this->indice;
	wprintf(L"[");
	for(i = 0; i < t; i++)
		wprintf(L"%s,", (wchar_t*) this->array_associativo[i]->chave);
	wprintf(L"]\n");
}

void* Vetor_find(Vetor* this, wchar_t* chave) {
	if(this->indice == 0)
		return NULL;
	ChaveValor* elemento =
		bsearch(&chave, this->array_associativo, this->indice,
			sizeof(ChaveValor*), ChaveValor_compare_key);
	if(elemento != NULL) {
		elemento = *(ChaveValor**) elemento;
		return elemento->valor;
	}
	return NULL;
}

/*
 * Função que adiciona um elemento à lista
 * lista: apontador para a estrutura que representa a lista
 * casa: apontador para a casa a adicionar à lista
 *
 * Devolve: true ou false caso tenha sido adicionado ou não
 */
bool Vetor_add(Vetor* this, wchar_t* chave, void* elemento) {
	// Se o indice for igual à capacidade significa que não
	// cabem mais elementos
	if(Vetor_find(this, chave) != NULL)
		return false;
	if(this->indice >= this->capacidade) {
		int nova_capacidade = ceil(this->capacidade * FATOR_EXPANSAO_VETOR);
		void* novo_sequencial = realloc(this->array_sequencial,
			nova_capacidade * this->tamanho_elemento);
		if(novo_sequencial == NULL)
			return false;
		void* novo_associativo = realloc(this->array_associativo,
			nova_capacidade * sizeof(ChaveValor*));
		if(novo_associativo == NULL)
			return false;
		this->capacidade = nova_capacidade;
		this->array_sequencial = novo_sequencial;
		this->array_associativo = novo_associativo;
	}
	// Adiciona e incrementa o indice
	this->array_sequencial[this->indice] = elemento;
	this->array_associativo[this->indice] = ChaveValor_new(chave, elemento, this->tamanho_elemento);
	this->indice++;
	qsort(this->array_associativo, this->indice, sizeof(ChaveValor*),
		ChaveValor_compare);
	return true;
}

/*
 * Função que devolve um apontador para um elemento aleatório da lista
 * lista: apontador para a estrutura que representa a lista
 *
 * Devolve: apontador para elemento aleatório ou NULL caso a lista esteja vazia
 */
void* Vetor_get(Vetor* this, int indice) {
	// Se a lista estiver vazia não é possível devolver uma casa
	if(this->indice > 0 && indice < this->indice)
		return this->array_sequencial[indice];
	return NULL;
}

typedef struct _grupo {
	wchar_t* nome;
	Vetor* equipas;
} Grupo;

typedef struct _equipa {
	wchar_t* nome;
	Grupo* grupo;
	Vetor* jogadores;
} Equipa;

typedef struct _jogador {
	wchar_t* nome;
	int golos;
} Jogador;

typedef struct _jogo {
	Equipa* equipa1;
	Equipa* equipa2;
} Jogo;

typedef struct _mundial {
	Vetor* grupos;
	Vetor* equipas;
	Vetor* jogadores;
	Vetor* jogos;
} Mundial;

Grupo* Grupo_new(wchar_t* nome) {
	Grupo* this = malloc(sizeof(Grupo));
	this->nome = malloc(wcslen(nome) * sizeof(wchar_t));
	wcscpy(this->nome, nome);
	this->equipas = Vetor_new(CAPACIDADE_GRUPOS, sizeof(Equipa*));
	return this;
}

Equipa* Equipa_new(wchar_t* nome, Grupo* grupo) {
	Equipa* this = malloc(sizeof(Equipa));
	this->nome = malloc(wcslen(nome) * sizeof(wchar_t));
	wcscpy(this->nome, nome);
	this->grupo = grupo;
	this->jogadores = Vetor_new(CAPACIDADE_JOGADORES, sizeof(Jogador*));
	return this;
}

void Equipa_ler(Mundial* mundial, wchar_t* linha) {
	wchar_t** partes = _str_split(linha, L" ", 2);
	if(partes != NULL) {
		Grupo* grupo = (Grupo*) Vetor_find(mundial->grupos, partes[0]);
		if(grupo == NULL)
			grupo = Grupo_new(partes[0]);
		Vetor_add(mundial->grupos, grupo->nome, grupo);

		Equipa* equipa = Equipa_new(partes[1], grupo);
		Vetor_add(grupo->equipas, equipa->nome, equipa);
		Vetor_add(mundial->equipas, equipa->nome, equipa);

		_free_array(partes, 2);
	}
}

Jogador* Jogador_new(wchar_t* nome) {
	Jogador* this = malloc(sizeof(Jogador));
	this->nome = malloc(wcslen(nome) * sizeof(wchar_t));
	wcscpy(this->nome, nome);
	return this;
}

void Jogador_ler(Mundial* mundial, wchar_t* linha) {
	wchar_t** partes = _str_split(linha, L"-", 2);
	if(partes != NULL) {
		//Vetor_print(mundial->equipas);
		wchar_t* nome_equipa = _str_trim(partes[0]);
		wchar_t* nome_jogador = _str_trim(partes[1]);
		Equipa* equipa = (Equipa*) Vetor_find(mundial->equipas, nome_equipa);
		wprintf(L">%s<\n", partes[1]);
		if(equipa != NULL) {
			Jogador* jogador = Jogador_new(partes[1]);
			Vetor_add(equipa->jogadores, jogador->nome, jogador);
			Vetor_add(mundial->jogadores, jogador->nome, jogador);
		} else {
			wprintf(L"Erro: equipa %s não encontrada, ao adicionar jogador %s\n", partes[0], partes[1]);
		}
		_free_array(partes, 2);
		free(nome_jogador);
		free(nome_equipa);
	}
}

Mundial* Mundial_new() {
	Mundial* this = malloc(sizeof(Mundial));
	this->grupos = Vetor_new(CAPACIDADE_GRUPOS, sizeof(Grupo*));
	this->equipas = Vetor_new(CAPACIDADE_EQUIPAS, sizeof(Equipa*));
	this->jogadores = Vetor_new(CAPACIDADE_JOGADORES, sizeof(Jogador*));
	this->jogos = Vetor_new(CAPACIDADE_JOGOS, sizeof(Jogo*));
	return this;
}

void Teste_A() {
	int i, t;

	wprintf(L"--\nAlinea a) Teste 1 - \n--\n");
	wchar_t* linhas1[7] = {
		L"A Brasil  ",
		L"B Chile  ",
		L" D Costa Rica",
		L"D Itália  ",
		L" E Equador",
		L"  ",
		L""
	};
	t = sizeof(linhas1) / sizeof(wchar_t*);
	for(i = 0; i < t; i++) {
		wprintf(L"\t[%s] -> ", linhas1[i]);
		_str_trim(linhas1[i]);
		wprintf(L"[%s]\n", linhas1[i]);
	}

	wprintf(L"--\nAlinea a) Teste 2 - _str_split\n--\n");
	wchar_t* linhas2[10] = {
		L"C Costa do Marfim",
		L" D Costa Rica",
		L"F Bósnia e Herzegovina",
		L"G  Alemanha",
		L"G 	Portugal",
		L"G  Gana",
		L"G  Estados Unidos",
		L"H Coreia do Sul",
		L"  ",
		L""
	};
	t = sizeof(linhas2) / sizeof(wchar_t*);
	int n_partes;
	for(n_partes = 1; n_partes <= 4; n_partes++) {
		wprintf(L"\t%d parte(s):\n", n_partes);
		for(i = 0; i < t; i++) {
			wprintf(L"\t\t[%s] -> ", linhas2[i]);
			_str_trim(linhas2[i]);
			wchar_t** partes =_str_split(linhas2[i], L" ", n_partes);
			if(partes != NULL) {
				int j;
				for(j = 0; j < n_partes; j++)
					wprintf(L"[%s]", partes[j]);
				wprintf(L"\n");
				_free_array(partes, n_partes);
			} else
				wprintf(L"Erro\n");
		}
	}
}

void Teste_B() {
	Vetor* vetor = Vetor_new(10, sizeof(wchar_t*));
	wchar_t* valor1 = L"valor1";
	wchar_t* valor2 = L"valor2";
	wchar_t* valor3 = L"valor3";
	wchar_t* valor4 = L"valor4";
	Vetor_add(vetor, valor1, valor1);
	Vetor_add(vetor, valor2, valor2);
	Vetor_add(vetor, valor3, valor3);
	Vetor_add(vetor, valor4, valor4);
	printf("%d\n", Vetor_length(vetor));
	printf("%d\n", vetor->capacidade);
	Vetor_print(vetor);
	wchar_t* res = (wchar_t*) Vetor_find(vetor, L"valor1");
	if(res != NULL)
		wprintf(L"%s", res);
	else
		printf("not found...");
}

wchar_t* _ler_linha(FILE* ficheiro) {
	wchar_t* linha = malloc(TAMANHO_BUFFER_LINHA * sizeof(wchar_t));
	if(ficheiro == NULL||fgetws(linha, TAMANHO_BUFFER_LINHA, ficheiro) == NULL){
		free(linha);
		return NULL;
	}
	_str_trim(linha);
	return linha;
}

void _ler_lista(Mundial* mundial, FILE* ficheiro, void (*funcao_leitura)(Mundial*, wchar_t*)) {
	if(ficheiro != NULL) {
		wchar_t* linha = NULL;
		while((linha = _ler_linha(ficheiro)) != NULL) {
			if(*linha == CARACTERE_FIM_LISTA)
				break;
			else {
				funcao_leitura(mundial, linha);
			}
		}
		if(linha == NULL)
			wprintf(L"Erro de leitura\n");
		free(linha);
	} else {
		wprintf(L"Erro ao aceder ao ficheiro\n");
	}
}

void Alinea_A(Mundial* mundial, FILE* ficheiro) {
	_ler_lista(mundial, ficheiro, Equipa_ler);
	int i, n_equipas = Vetor_length(mundial->equipas);
	for(i = 0; i < n_equipas; i++) {
		Equipa* equipa = (Equipa*) Vetor_get(mundial->equipas, i);
		if(equipa != NULL)
			wprintf(L"%s\n", equipa->nome);
	}
	wprintf(L"--\nEquipas: %d", n_equipas);
}

void Alinea_B(Mundial* mundial, FILE* ficheiro) {
	_ler_lista(mundial, ficheiro, Equipa_ler);
	_ler_lista(mundial, ficheiro, Jogador_ler);
	int i, j,
		n_grupos = Vetor_length(mundial->grupos),
		total_equipas = 0;
	for(i = 0; i < n_grupos; i++) {
		Grupo* grupo = (Grupo*) Vetor_get(mundial->grupos, i);
		if(grupo != NULL) {
			int n_equipas = Vetor_length(grupo->equipas);
			for(j = 0; j < n_equipas; j++) {
				Equipa* equipa = (Equipa*) Vetor_get(grupo->equipas, j);
				if(equipa != NULL) {
					total_equipas++;
					wprintf(L"%s\n", equipa->nome);
				}
			}
		}
	}
	wprintf(L"--\nEquipas: %d", total_equipas);
}

int main(int argc, char** argv) {
	Mundial* mundial = Mundial_new();

	char alinea = 0;
	char* nome_ficheiro = NULL;

	if(argc == 1) {
		wprintf(L"Modo de utilização:\n\n");
		wprintf(L"\t1200781 A alinea nome_ficheiro_dados\texecuta uma alínea\n");
		wprintf(L"\t1200781 T alinea nome_ficheiro_dados\tefetua testes relativos a uma alínea\n");
		wprintf(L"\t1200781 nome_ficheiro_dados\t\tequivalente a 1200781 A D (executa alínea D)\n");
	} else if(argc == 3) {
		alinea = tolower(*argv[1]);
		nome_ficheiro = argv[2];
	} else if(argc == 2) {
		alinea = L'd';
		nome_ficheiro = argv[1];
	}

	FILE* ficheiro = fopen(nome_ficheiro, "r");
	if(ficheiro != NULL) {
		switch(alinea) {
			case L'a':
				Alinea_A(mundial, ficheiro);
				break;
			case L'b':
				Alinea_B(mundial, ficheiro);
				break;
			case L't':
				Teste_A();
				break;
		}
	}
	return EXIT_SUCCESS;
}

#include "stego.h"

encrypted_r2_vector::encrypted_r2_vector(int m, const Eigen::VectorXi & start, const Eigen::VectorXi & mess) {
	this->m = m;
	this->start_vector = start;
	this->message = mess;
	this->matrix_H = create_matrix_H();
}

encrypted_r1_vector::encrypted_r1_vector(int m, const Eigen::VectorXi & start, const Eigen::VectorXi & mess) {
	this->m = m;
	this->start_vector = start;
	this->message = mess;
	this->matrix_H = create_matrix_H();
}

Eigen::MatrixXi encrypted_r2_vector::create_matrix_H() {
	Eigen::MatrixXi H = encrypted_vector::create_matrix_H();
	Eigen::MatrixXi Hnew(H.rows() + (m - 1)*m / 2, H.cols());
	for (int i = 0; i < H.rows(); i++)
		for (int j = 0; j < H.cols(); j++)
			Hnew(i, j) = H(i, j);
	H = Hnew;
	int ind = m;
	for (int i = 1; i <= m; i++)
	for (int j = i + 1; j <= m; j++) {
		++ind;
		H.block(ind, 0, 1, H.cols()) = (H.block(i, 0, 1, H.cols()) + H.block(j, 0, 1, H.cols())) / 2;
	}
	int elem = -1;
	return H;
}

Eigen::MatrixXi encrypted_vector::create_matrix_H() {
	int n = pow(2, m);
	int k = 1 + m;
	Eigen::MatrixXi H(k, n);
	for (int i = 0; i < H.cols(); i++) {
		H(0, i) = 1;
		std::vector<bool> value = binary(i, m);
		for (int j = 0; j < value.size(); j++)
			H(j + 1, i) = value[j];
	}
	return H;
}

std::vector<bool> binary(unsigned value, int count_num) {
	std::vector<bool> bin_value = std::vector<bool>();
	for (int i = 0; value > 0; i++)
	{
		bin_value.push_back(value % 2);
		value = value / 2;
	}
	while (bin_value.size() < count_num)
		bin_value.push_back(0);
	return std::vector<bool>(bin_value.rbegin(), bin_value.rend());
}

int encrypted_vector::weight(const Eigen::VectorXi & v) const{
	int result = 0;
	for (int i = 0; i < v.size(); i++)
	if (v[i] != 0) result++;
	return result;
}

std::vector<int> encrypted_r2_vector::ind_uniq() {
	std::vector<int> ind = std::vector<int>();
	for (int i = 0; i < m; i++)
		ind.push_back(pow(2, i));
	for (int i = 1; i < m; i++)
	for (int j = 0; j < i; j++)
		ind.push_back(ind[i] + ind[j]);
	std::rotate(ind.begin(), ind.begin() + m, ind.end());
	return ind;
}

unsigned decimal(const Eigen::VectorXi & x, bool source) {
	unsigned value = 0;
	int count = source == true ? x.size() - 1 : x.size();
	for (int i = 0; i < count; i++) 
		value += pow(2, i) * x[x.size() - i - 1];
	assert(value >= 0);
	return value;
}

std::vector<int> encrypted_r2_vector::analysis(const Eigen::VectorXi & v) {
	std::vector<int> result = std::vector<int>();
	std::vector<int> tmp = ind_uniq();
	int min = pow(2, m);
	int coverage_r = m % 2 == 1 ? m + 1 : m + 2;
	std::vector<int> start_ind = std::vector<int>();
	for (int i = 0; i < matrix_H.cols(); i++)
		start_ind.push_back(i);
	for (int i = 0; i < matrix_H.cols() - 1; i++) 
		for(int j = matrix_H.cols() - 1; j >= i + 1; j--) {
			int curr_weight1 = abs(weight(v.segment(m + 1, v.size() - (m + 1))) - weight(matrix_H.block(m + 1, j, matrix_H.rows() - (m + 1), 1)));
			int curr_weight2 = abs(weight(v.segment(m + 1, v.size() - (m + 1))) - weight(matrix_H.block(m + 1, j - 1, matrix_H.rows() - (m + 1), 1)));
			if (curr_weight1 < curr_weight2)
				std::swap(start_ind[j], start_ind[j - 1]);
	}
	int ind = 0;
	while (true) {
		if (ind >= start_ind.size()) 
			return result;
		result.push_back(start_ind[ind]);
		Eigen::VectorXi curr_vect = matrix_H.block(0, start_ind[ind], matrix_H.rows(), 1);
		for (int i = curr_vect.size() - 1; i > 0; i--) {
			if (curr_vect[i] != v[i] && std::find(result.begin(), result.end(), tmp[tmp.size() - i]) == result.end()) {
				curr_vect += matrix_H.block(0, tmp[tmp.size() - i], matrix_H.rows(), 1);
				curr_vect -= (curr_vect / 2) * 2;
				result.push_back(tmp[tmp.size() - i]);
			}
		}
		if (curr_vect[0] != v[0] && std::find(result.begin(), result.end(), 0) == result.end()) {
			curr_vect += matrix_H.block(0, 0, matrix_H.rows(), 1);
			curr_vect -= (curr_vect / 2) * 2;
			result.push_back(0);
		}
		if (result.size() <= coverage_r && curr_vect == v) {
			return result;
		}
		result.clear();
		ind++;
	}
}

std::vector<int> encrypted_r1_vector::analysis(const Eigen::VectorXi & v){
	int value = decimal(v, true);
	std::vector<int> result = std::vector<int>();
	if (v[0] == 1)
		result.push_back(value);
	else
	while (true) {
		int i, j;
		i = rand() % value;
		Eigen::VectorXi i_val = matrix_H.col(i);
		Eigen::VectorXi j_val = v - i_val;
		for (int i = 0; i < j_val.size(); i++) {
			j_val[i] %= 2;
			j_val[i] *= j_val[i];
		}
		j = decimal(j_val, true);
		if (i != j) {
			result.push_back(i);
			result.push_back(j);
			break;
		}
		else
			continue;
	}
	return result;
}

Eigen::VectorXi encrypted_vector::get_vector_x() {
	Eigen::VectorXi y(matrix_H.rows());
	y = start_vector.transpose() * matrix_H.transpose();
	Eigen::VectorXi x(y.size());
	assert(message.size() == y.size());
	x = message - y;
	for (int i = 0; i < x.size(); i++) {
		x[i] %= 2;
		x[i] *= x[i];
	}
	return x;
}

Eigen::VectorXi encrypted_vector::get_encrypted_vector() {
	Eigen::VectorXi x = get_vector_x();
	Eigen::VectorXi zed(x.size());
	for (int i = 0; i < zed.size(); i++)
		zed[i] = 0;
	if (x == zed) 
		return start_vector;
	Eigen::VectorXi epsilon = get_eps(analysis(x));
	Eigen::VectorXi result = start_vector + epsilon;
	result -= (result / 2) * 2;
	return result;
}

Eigen::VectorXi encrypted_vector::get_eps(const std::vector<int> & ind) {
	int size = pow(2, m);
	Eigen::VectorXi eps(size);
	for (int i = 0; i < size; i++)
		eps[i] = 0;
	for (int i = 0; i < ind.size(); i++) {
		eps[ind[i]] = 1;
	}
	return eps;
}

translator::translator(std::wstring text, int size) {
	this->text = text;
	this->size = size;
	create_dictionary();
	this->message = translate_text();
}

translator::translator(Eigen::VectorXi & message, int size) {
	this->message = message;
	this->size = size;
	create_dictionary();
	this->text = translate_vector();
}

void translator::create_dictionary() {
	dictionary_ch_to_vec = std::map<char, std::vector<bool>>();
	dictionary_vec_to_ch = std::map<std::vector<bool>, char>();
	for each (char c in punctual) {
		std::vector<bool> char_vector = binary(dictionary_ch_to_vec.size(), size);
		dictionary_ch_to_vec.insert(std::pair<char, std::vector<bool>>(c, char_vector));
		dictionary_vec_to_ch.insert(std::pair<std::vector<bool>, char>(char_vector, c));
	}
	for (char c = 'A'; c <= 'Z'; c++) {
		std::vector<bool> char_vector = binary(dictionary_ch_to_vec.size(), size);
		dictionary_ch_to_vec.insert(std::pair<char, std::vector<bool>>(c, char_vector));
		dictionary_vec_to_ch.insert(std::pair<std::vector<bool>, char>(char_vector, c));
	}
	for (char c = 'a'; c <= 'z'; c++) {
		std::vector<bool> char_vector = binary(dictionary_ch_to_vec.size(), size);
		dictionary_ch_to_vec.insert(std::pair<char, std::vector<bool>>(c, char_vector));
		dictionary_vec_to_ch.insert(std::pair<std::vector<bool>, char>(char_vector, c));
	}
	for (char c = '�'; c <= '�'; c++) {
		std::vector<bool> char_vector = binary(dictionary_ch_to_vec.size(), size);
		dictionary_ch_to_vec.insert(std::pair<char, std::vector<bool>>(c, char_vector));
		dictionary_vec_to_ch.insert(std::pair<std::vector<bool>, char>(char_vector, c));
	}
	for (char c = '�'; c <= '�'; c++) {
		std::vector<bool> char_vector = binary(dictionary_ch_to_vec.size(), size);
		dictionary_ch_to_vec.insert(std::pair<char, std::vector<bool>>(c, char_vector));
		dictionary_vec_to_ch.insert(std::pair<std::vector<bool>, char>(char_vector, c));
	}
	for (char c = '0'; c <= '9'; c++) {
		std::vector<bool> char_vector = binary(dictionary_ch_to_vec.size(), size);
		dictionary_ch_to_vec.insert(std::pair<char, std::vector<bool>>(c, char_vector));
		dictionary_vec_to_ch.insert(std::pair<std::vector<bool>, char>(char_vector, c));
	}
	assert(dictionary_ch_to_vec.size() <= pow(2, size));
}

Eigen::VectorXi translator::translate_text() {
	Eigen::VectorXi text_matr(0);
	int num_row = 0;
	for each (char c in text)
	{
		if (!isdigit(c) && !isalpha(c) && punctual.find(c) == punctual.end()) continue;
		std::vector<bool> value = dictionary_ch_to_vec.at(c);
		text_matr.conservativeResize(text_matr.rows() + size);
		for (int j = 0; j < value.size(); j++) {
			text_matr(num_row + j) = value[j];
		}
		num_row = text_matr.rows();
	}
	return text_matr;
}

std::wstring translator::translate_vector() {
	std::wstring result;
	int index = 0;
	while (true) {
		std::vector<bool> symbol(size);
		for (int i = index; i < index + size; i++) {
			if (i >= message.size()) break;
			symbol[i % size] = message(i);
		}
		result += (char)dictionary_vec_to_ch[symbol];
		index += size;
		if (index >= message.size()) break;
	}
	return result;
}

decrypted_r2_vector::decrypted_r2_vector(int m, Eigen::VectorXi & encrypted_vector) {
	int n = pow(2, m);
	assert(encrypted_vector.size() == n);
	this->encrypted_vector = encrypted_vector;
	this->m = m;
	this->matrix_H = create_matrix_H();
}

decrypted_r1_vector::decrypted_r1_vector(int m, Eigen::VectorXi & encrypted_vector) {
	int n = pow(2, m);
	assert(encrypted_vector.size() == n);
	this->encrypted_vector = encrypted_vector;
	this->m = m;
	this->matrix_H = decrypted_vector::create_matrix_H();
}

Eigen::MatrixXi decrypted_vector::create_matrix_H() {
	int n = pow(2, m);
	int k = 1 + m;
	Eigen::MatrixXi H(k, n);
	for (int i = 0; i < H.cols(); i++) {
		H(0, i) = 1;
		std::vector<bool> value = binary(i, m);
		for (int j = 0; j < value.size(); j++)
			H(j + 1, i) = value[j];
	}
	return H;
}

Eigen::MatrixXi decrypted_r2_vector::create_matrix_H() {
	Eigen::MatrixXi H = decrypted_vector::create_matrix_H();
	Eigen::MatrixXi Hnew(H.rows() + (m - 1)*m / 2, H.cols());
	for (int i = 0; i < H.rows(); i++)
		for (int j = 0; j < H.cols(); j++)
			Hnew(i, j) = H(i, j);
	H = Hnew;
	int ind = m;
	for (int i = 1; i <= m; i++)
		for (int j = i + 1; j <= m; j++) {
			++ind;
			H.block(ind, 0, 1, H.cols()) = (H.block(i, 0, 1, H.cols()) + H.block(j, 0, 1, H.cols())) / 2;
		}
	int elem = -1;
	return H;
}

Eigen::VectorXi decrypted_vector::get_decrypted_vector() {
	Eigen::VectorXi result = encrypted_vector.transpose() * matrix_H.transpose();
	result -= (result / 2) * 2;
	return result;
}
let data = null;
const addBtn = document.querySelector('.bi-plus-square');
const saveBtn = document.querySelector('.btn-save');
const tBody = document.querySelector('tbody');
const tableRows = tBody.getElementsByTagName('tr');
let infoString = document.querySelector('.info-string');
let showRowBtns = true;
let rowIconDone = false;
let editText = '';
let cancelBtn = false;

function loadData() {
  const xhttp = new XMLHttpRequest();
  const url = "get_webradio.cgi";
  xhttp.onload = function() {
    data = this.responseText.split('\r\n');
    updateTable();
    saveBtn.disabled = true;
  }
  xhttp.open("GET", url, true);
  xhttp.send();
}

function saveData() {
  const xhttp = new XMLHttpRequest();
  const url = "post_webradio.cgi";
  xhttp.onload = function() {
    infoString.innerHTML = this.responseText;
    saveBtn.disabled = true;
  }
  xhttp.open("POST", url, true);
  xhttp.setRequestHeader("Content-Type", "application/json; charset=utf-8");
  xhttp.send(data.join('\r\n'));
}

function updateTable() {
    const tableData = data
        .map((item) => {
            return `
    <tr draggable="true" ondragstart="rowDragStart()" ondragover="rowDragOver()">
    <td class="edit-cell-1 single-line">${item}</td>
    <td><i class="bi-pencil-square h4"></i>&nbsp;<i class="bi-trash h4"></i></td>
    </tr>
    `;
        })
        .join('');

  tBody.innerHTML = tableData;

  toggleEditBtn();
  toggleDeleteBtn();
}

loadData();

function addItem() {
  data.push('');
  updateTable();

  let newTr = tBody.rows[tableRows.length - 1];
  let editCell = newTr.querySelector('td.edit-cell-1');
  let firstIcon = newTr.querySelector('.bi-pencil-square');
  let secondIcon = newTr.querySelector('.bi-trash');
  startEdit(editCell, firstIcon, secondIcon, newTr);
}

function saveEdit(editCell, firstIcon, secondIcon) {
  firstIcon.classList.remove('bi-check-square');
  firstIcon.classList.add('bi-pencil-square');
  secondIcon.classList.remove('bi-x-square');
  secondIcon.classList.add('bi-trash');
  editCell.removeAttribute('contenteditable', true);

  let children = Array.from(editCell.parentNode.parentNode.children);
  let index = children.indexOf(editCell.parentNode);
  data[index] = editCell.innerText;
  rowIconDone = true;
  cancelBtn = false;
  saveBtn.disabled = false;
  infoString.innerHTML = "";
}

function startEdit(editCell, firstIcon, secondIcon, newTr) {
  firstIcon.classList.remove('bi-pencil-square');
  firstIcon.classList.add('bi-check-square');
  secondIcon.classList.remove('bi-trash');
  secondIcon.classList.add('bi-x-square');
  editText = editCell.innerText;
  editCell.setAttribute('contenteditable', true);
  editCell.focus();
  rowIconDone = false;
  showRowBtns = false;
  editCell.addEventListener('blur', function handler() {
    setTimeout(function() {
      showRowBtns = true;
      if (rowIconDone === false) {
        // Cancel edit
        if (newTr === undefined) {
          firstIcon.classList.remove('bi-check-square');
          firstIcon.classList.add('bi-pencil-square');
          secondIcon.classList.remove('bi-x-square');
          secondIcon.classList.add('bi-trash');
          editCell.removeAttribute('contenteditable', true);
          editCell.innerHTML = editText;
        } else {
          deleteRow(newTr);
          cancelBtn = false;
        }
        rowIconDone = true;
      }
      editCell.removeEventListener('blur', handler);
    }, 0);
  });
  editCell.addEventListener('keydown', function handler(event) {
    if (event.key === 'Enter') {
      event.preventDefault();
      // Save edit
      firstIcon = event.target.parentElement.querySelector('.bi-check-square');
      secondIcon = event.target.parentElement.querySelector('.bi-x-square');
      saveEdit(editCell, firstIcon, secondIcon);
      editCell.removeEventListener('keydown', handler);
    }
    if (event.key === 'Escape') {
      editCell.blur();
      editCell.removeEventListener('keydown', handler);
	}
  });
  secondIcon.addEventListener('mousedown', function handler() {
    if (secondIcon.classList.contains('bi-x-square')) {
	  cancelBtn = true;
	}
    secondIcon.removeEventListener('mousedown', handler);
  });
}

function deleteRow(deleteTr) {
  const deleteIndex = deleteTr.rowIndex - 1;
  deleteTr.remove();
  data.splice(deleteIndex, 1);
  if (data.length > 0) {
    saveBtn.disabled = false;
  } else {
    saveBtn.disabled = true;
  }
}

function clickFirstRowIcon() {
  let firstIcon;
  let secondIcon;
  let editCell = this.parentElement.parentElement.querySelector('td.edit-cell-1');
  firstIcon = this.parentElement.querySelector('.bi-pencil-square');
  if (firstIcon === null) {
    // Save edit
    firstIcon = this.parentElement.querySelector('.bi-check-square');
    secondIcon = this.parentElement.querySelector('.bi-x-square');
    saveEdit(editCell, firstIcon, secondIcon);
  } else {
    // Start edit
    secondIcon = this.parentElement.querySelector('.bi-trash');
    startEdit(editCell, firstIcon, secondIcon);
  }
}

function clickSecondRowIcon() {
  let secondIcon = this.parentElement.querySelector('.bi-trash');
  if (secondIcon != null && cancelBtn === false) {
    // Delete row
    deleteRow(this.parentElement.parentElement);
  }
  cancelBtn = false;
}

function toggleDeleteBtn() {
  for (let tr of tableRows) {
    const deleteIcon = tr.querySelector('.bi-trash');
    tr.addEventListener('mouseover', () => {
      if (showRowBtns === true) {
        deleteIcon.classList.add('showDeleteIcon');
      }
    });
    tr.addEventListener('mouseleave', () => {
      deleteIcon.classList.remove('showDeleteIcon');
    });
    deleteIcon.addEventListener('click', clickSecondRowIcon);
  }
}

function toggleEditBtn() {
  for (let tr of tableRows) {
    const editIcon = tr.querySelector('.bi-pencil-square');
    tr.addEventListener('mouseover', () => {
      if (showRowBtns === true) {
        editIcon.classList.add('showEditIcon');
      }
    });
    tr.addEventListener('mouseleave', () => {
      editIcon.classList.remove('showEditIcon');
    });
    editIcon.addEventListener('mousedown', () => {
      event.preventDefault();
    });
    editIcon.addEventListener('click', clickFirstRowIcon);
  }
}

addBtn.addEventListener('click', addItem);
saveBtn.addEventListener('click', saveData);

function rowDragStart(){  
  row = event.target; 
}

function rowDragOver(){
  event.preventDefault(); 
  
  let children = Array.from(event.target.parentNode.parentNode.children);
  let movRowIndex = children.indexOf(row);
  let swpRowIndex = children.indexOf(event.target.parentNode);
  
  if (swpRowIndex > movRowIndex) {
    event.target.parentNode.after(row);
    // swap objects
    [data[swpRowIndex], data[movRowIndex]] = [data[movRowIndex], data[swpRowIndex]];
    saveBtn.disabled = false;
    infoString.innerHTML = "";
  } else if (swpRowIndex < movRowIndex) {
    event.target.parentNode.before(row);
    // swap objects
    [data[movRowIndex], data[swpRowIndex]] = [data[swpRowIndex], data[movRowIndex]];
    saveBtn.disabled = false;
    infoString.innerHTML = "";
  }
}

#include "certificatemanagementdialog.hpp"
#include "ui_certificatemanagementdialog.h"

#include "kristall.hpp"

#include "newidentitiydialog.hpp"

#include <QCryptographicHash>
#include <QMessageBox>

CertificateManagementDialog::CertificateManagementDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CertificateManagementDialog),
    selected_identity { nullptr }
{
    ui->setupUi(this);

    this->ui->certificates->setModel(&global_identities);
    this->ui->certificates->expandAll();

    connect(
        this->ui->certificates->selectionModel(),
        &QItemSelectionModel::currentChanged,
        this,
        &CertificateManagementDialog::on_certificates_selected
    );
    on_certificates_selected(QModelIndex { }, QModelIndex { });
}

CertificateManagementDialog::~CertificateManagementDialog()
{
    delete ui;
}

void CertificateManagementDialog::on_certificates_selected(QModelIndex const& index, QModelIndex const & previous)
{
    Q_UNUSED(previous);

    selected_identity = global_identities.getMutableIdentity(index);

    this->ui->export_cert_button->setEnabled(selected_identity != nullptr);

    if(selected_identity != nullptr)
    {
        auto & cert = *selected_identity;
        this->ui->groupBox->setEnabled(true);
        this->ui->cert_display_name->setText(cert.display_name);
        this->ui->cert_common_name->setText(cert.certificate.subjectInfo(QSslCertificate::CommonName).join(", "));
        this->ui->cert_expiration_date->setDateTime(cert.certificate.expiryDate());
        this->ui->cert_livetime->setText(QString("%1 days").arg(QDateTime::currentDateTime().daysTo(cert.certificate.expiryDate())));
        this->ui->cert_fingerprint->setPlainText(
            QCryptographicHash::hash(cert.certificate.toDer(), QCryptographicHash::Sha256).toHex(':')
        );
        this->ui->cert_notes->setPlainText(cert.user_notes);

        this->ui->delete_cert_button->setEnabled(true);
    }
    else
    {
        this->ui->groupBox->setEnabled(false);
        this->ui->cert_display_name->setText("");
        this->ui->cert_common_name->setText("");
        this->ui->cert_expiration_date->setDateTime(QDateTime { });
        this->ui->cert_livetime->setText("");
        this->ui->cert_fingerprint->setPlainText("");

        if(auto group_name = global_identities.group(index); not group_name.isEmpty()) {
            this->ui->delete_cert_button->setEnabled(global_identities.canDeleteGroup(group_name));
        } else {
            this->ui->delete_cert_button->setEnabled(false);
        }
    }
}

void CertificateManagementDialog::on_cert_notes_textChanged()
{
    if(this->selected_identity != nullptr) {
        this->selected_identity->user_notes = this->ui->cert_notes->toPlainText();
    }
}

void CertificateManagementDialog::on_cert_display_name_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1)
    if(this->selected_identity != nullptr) {
        this->selected_identity->display_name = this->ui->cert_display_name->text();
    }
}

void CertificateManagementDialog::on_delete_cert_button_clicked()
{
    auto index = this->ui->certificates->currentIndex();

    if(global_identities.getMutableIdentity(index) != nullptr)
    {
        auto answer = QMessageBox::question(
            this,
            "Kristall",
            "Do you really want to delete this certificate?\r\n\r\nYou will not be able to restore the identity after this!",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        if(answer != QMessageBox::Yes)
            return;
        if(not global_identities.destroyIdentity(index)) {
            QMessageBox::warning(this, "Kristall", "Could not destroy identity!");
        }
    }
    else if(auto group_name = global_identities.group(index); not group_name.isEmpty()) {

        auto answer = QMessageBox::question(
            this,
            "Kristall",
            QString("Do you want to delete the group '%1'").arg(group_name)
        );
        if(answer != QMessageBox::Yes)
            return;

        if(not global_identities.deleteGroup(group_name)) {
            QMessageBox::warning(this, "Kristall", "Could not delete group!");
        }
    }
}

void CertificateManagementDialog::on_export_cert_button_clicked()
{

}

void CertificateManagementDialog::on_import_cert_button_clicked()
{

}

void CertificateManagementDialog::on_create_cert_button_clicked()
{
    NewIdentitiyDialog dialog { this };

    dialog.setGroupName(global_identities.group(this->ui->certificates->currentIndex()));

    if(dialog.exec() != QDialog::Accepted)
        return;

    auto id = dialog.createIdentity();
    if(not id.isValid())
        return;
    id.is_persistent = true;

    global_identities.addCertificate(
        dialog.groupName(),
        id);
}
